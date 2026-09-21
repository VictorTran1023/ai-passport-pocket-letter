#include "sp_ble.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <stdatomic.h>
#include <string.h>
/* Protocol v1: 1 selects a download offset; 2 uploads total/offset/payload;
 * 3 selects persistence status; 4 confirms the initiator saved its received card.
 * Every ATT value fits the mandatory MTU 23. Peers do not need MTU negotiation. */
static const ble_uuid128_t service_uuid = BLE_UUID128_INIT(
    0x81, 0x72, 0x63, 0x54, 0x45, 0x36, 0x27, 0x18, 0x9a, 0xbc, 0xde, 0xf0, 0x01, 0x53, 0x50, 0xfa);
static const ble_uuid128_t value_uuid = BLE_UUID128_INIT(
    0x81, 0x72, 0x63, 0x54, 0x45, 0x36, 0x27, 0x18, 0x9a, 0xbc, 0xde, 0xf0, 0x02, 0x53, 0x50, 0xfa);
static bool initialized, connecting, central, synced;
static atomic_bool running, ready;
static uint8_t address_type, own_id[8], peer_id[8], adv[17];
static uint8_t own_wire[SP_WIRE_MAX];
static uint16_t own_len, remote_handle;
/* Stop runs on the app worker while GAP callbacks run on the host task. */
static atomic_uint conn = BLE_HS_CONN_HANDLE_NONE;
static uint16_t offset, read_offset, last_length;
static uint32_t generation;
static atomic_uint receipt;
static uint8_t read_mode, rx_status;
static sp_rx_t rx;
static bool submitted, success;
static int64_t session_started, next_attempt;
static sp_ble_receive_fn receive_fn;
static SemaphoreHandle_t stopped;
static TaskHandle_t host_handle;
static struct ble_npl_callout tick_callout;
typedef enum {
    IDLE,
    DISCOVER,
    UPLOAD,
    STATUS_SELECT,
    STATUS_READ,
    DOWNLOAD_SELECT,
    DOWNLOAD_READ,
    WAIT_LOCAL,
    FINISH
} phase_t;
static phase_t phase;
static bool waiting;
static struct {
    uint8_t id[8];
    int64_t until;
} cooldown[16];
static unsigned cooldown_pos;
static int gap(struct ble_gap_event *event, void *arg);
static void radio_idle(void);
static void send_next(void);
static int write_done(uint16_t h, const struct ble_gatt_error *error, struct ble_gatt_attr *attr,
                      void *arg);
static int read_done(uint16_t h, const struct ble_gatt_error *error, struct ble_gatt_attr *attr,
                     void *arg);
static uint16_t u16(const uint8_t *p) {
    return (uint16_t)p[0] | (uint16_t)p[1] << 8;
}
static void put16(uint8_t *p, uint16_t n) {
    p[0] = (uint8_t)n;
    p[1] = (uint8_t)(n >> 8);
}
static void end_connection(void) {
    if (conn != BLE_HS_CONN_HANDLE_NONE)
        ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM);
}
static bool current(uint16_t h, void *arg) {
    return atomic_load(&running) && h == conn && (uint32_t)(uintptr_t)arg == generation;
}
static void submit(void) {
    if (submitted)
        return;
    sp_card_t card;
    if (!sp_decode(&card, rx.data, rx.total) || !memcmp(card.id, own_id, 8) ||
        (central && memcmp(card.id, peer_id, 8))) {
        rx_status = 2;
        end_connection();
        return;
    }
    memcpy(peer_id, card.id, 8);
    submitted = true;
    if (!receive_fn(generation, rx.data, rx.total)) {
        rx_status = 2;
        end_connection();
    }
}
static int access(uint16_t h, uint16_t attr, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    (void)attr;
    (void)arg;
    if (h != conn || central)
        return BLE_ATT_ERR_UNLIKELY;
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint8_t packet[18];
        size_t n;
        if (read_mode == 3) {
            packet[0] = rx_status;
            n = 1;
        } else {
            if (read_offset >= own_len)
                return BLE_ATT_ERR_INVALID_OFFSET;
            n = own_len - read_offset;
            if (n > SP_CHUNK_SIZE)
                n = SP_CHUNK_SIZE;
            put16(packet, own_len);
            put16(packet + 2, read_offset);
            memcpy(packet + 4, own_wire + read_offset, n);
            n += 4;
        }
        return os_mbuf_append(ctxt->om, packet, n) ? BLE_ATT_ERR_INSUFFICIENT_RES : 0;
    }
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
        return BLE_ATT_ERR_UNLIKELY;
    uint8_t data[20];
    uint16_t n = 0;
    if (OS_MBUF_PKTLEN(ctxt->om) > sizeof data ||
        ble_hs_mbuf_to_flat(ctxt->om, data, sizeof data, &n))
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    if (n == 3 && data[0] == 1) {
        read_mode = 1;
        read_offset = u16(data + 1);
        return read_offset < own_len ? 0 : BLE_ATT_ERR_INVALID_OFFSET;
    }
    if (n == 1 && data[0] == 3) {
        read_mode = 3;
        return 0;
    }
    if (n == 1 && data[0] == 4 && rx_status == 1) {
        success = true;
        return 0;
    }
    if (n >= 6 && data[0] == 2) {
        int r = sp_rx_chunk(&rx, u16(data + 3), u16(data + 1), data + 5, n - 5);
        if (r < 0)
            return BLE_ATT_ERR_INVALID_OFFSET;
        if (r == 1)
            submit();
        return 0;
    }
    return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
}
static const struct ble_gatt_svc_def services[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &service_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){{.uuid = &value_uuid.u,
                                      .access_cb = access,
                                      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE},
                                     {0}}},
    {0}};
static void send_next(void) {
    if (waiting || !central || conn == BLE_HS_CONN_HANDLE_NONE)
        return;
    uint8_t data[20];
    size_t n = 0;
    if (phase == UPLOAD) {
        last_length = own_len - offset;
        if (last_length > SP_CHUNK_SIZE)
            last_length = SP_CHUNK_SIZE;
        data[0] = 2;
        put16(data + 1, own_len);
        put16(data + 3, offset);
        memcpy(data + 5, own_wire + offset, last_length);
        n = 5 + last_length;
    } else if (phase == STATUS_SELECT) {
        data[0] = 3;
        n = 1;
    } else if (phase == DOWNLOAD_SELECT) {
        data[0] = 1;
        put16(data + 1, rx.used);
        n = 3;
    } else if (phase == FINISH) {
        data[0] = 4;
        n = 1;
    } else
        return;
    waiting = true;
    int rc = ble_gattc_write_flat(conn, remote_handle, data, n, write_done,
                                  (void *)(uintptr_t)generation);
    if (rc) {
        waiting = false;
        end_connection();
    }
}
static int read_done(uint16_t h, const struct ble_gatt_error *error, struct ble_gatt_attr *attr,
                     void *arg) {
    if (!current(h, arg))
        return 0;
    waiting = false;
    if (error->status || !attr) {
        end_connection();
        return 0;
    }
    uint8_t data[20];
    uint16_t n = 0;
    if (OS_MBUF_PKTLEN(attr->om) > sizeof data ||
        ble_hs_mbuf_to_flat(attr->om, data, sizeof data, &n)) {
        end_connection();
        return 0;
    }
    if (phase == STATUS_READ) {
        if (n != 1 || data[0] > 1) {
            end_connection();
            return 0;
        }
        if (!data[0]) {
            phase = STATUS_SELECT;
            return 0;
        }
        phase = DOWNLOAD_SELECT;
        send_next();
    } else if (phase == DOWNLOAD_READ) {
        if (n < 5) {
            end_connection();
            return 0;
        }
        int r = sp_rx_chunk(&rx, u16(data + 2), u16(data), data + 4, n - 4);
        if (r < 0) {
            end_connection();
            return 0;
        }
        if (r == 1) {
            phase = WAIT_LOCAL;
            submit();
        } else {
            phase = DOWNLOAD_SELECT;
            send_next();
        }
    }
    return 0;
}
static int write_done(uint16_t h, const struct ble_gatt_error *error, struct ble_gatt_attr *attr,
                      void *arg) {
    (void)attr;
    if (!current(h, arg))
        return 0;
    waiting = false;
    if (error->status) {
        end_connection();
        return 0;
    }
    if (phase == UPLOAD) {
        offset += last_length;
        if (offset == own_len)
            phase = STATUS_SELECT;
        send_next();
    } else if (phase == STATUS_SELECT || phase == DOWNLOAD_SELECT) {
        phase = phase == STATUS_SELECT ? STATUS_READ : DOWNLOAD_READ;
        waiting = true;
        if (ble_gattc_read(conn, remote_handle, read_done, arg)) {
            waiting = false;
            end_connection();
        }
    } else if (phase == FINISH) {
        success = true;
        end_connection();
    }
    return 0;
}
static int discovered(uint16_t h, const struct ble_gatt_error *error,
                      const struct ble_gatt_chr *chr, void *arg) {
    if (!current(h, arg))
        return 0;
    if (!error->status && chr) {
        if (remote_handle) {
            end_connection();
            return BLE_HS_EINVAL;
        }
        remote_handle = chr->val_handle;
    } else if (error->status == BLE_HS_EDONE && remote_handle) {
        phase = UPLOAD;
        send_next();
    } else if (error->status) {
        end_connection();
    }
    return 0;
}
static void radio_idle(void) {
    if (!atomic_load(&running) || !synced || conn != BLE_HS_CONN_HANDLE_NONE || connecting)
        return;
    if (!ble_gap_adv_active()) {
        struct ble_hs_adv_fields fields = {0};
        fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
        fields.mfg_data = adv;
        fields.mfg_data_len = sizeof adv;
        struct ble_gap_adv_params params = {0};
        params.conn_mode = BLE_GAP_CONN_MODE_UND;
        params.disc_mode = BLE_GAP_DISC_MODE_GEN;
        params.itvl_min = 320;
        params.itvl_max = 480;
        if (ble_gap_adv_set_fields(&fields) == 0)
            ble_gap_adv_start(address_type, NULL, BLE_HS_FOREVER, &params, gap, NULL);
    }
    if (!ble_gap_disc_active() && esp_timer_get_time() >= next_attempt) {
        struct ble_gap_disc_params p = {0};
        p.passive = 1;
        p.itvl = 160;
        p.window = 80;
        p.filter_duplicates = 0;
        ble_gap_disc(address_type, BLE_HS_FOREVER, &p, gap, NULL);
    }
    atomic_store(&ready, ble_gap_adv_active());
}
static int gap(struct ble_gap_event *e, void *arg) {
    (void)arg;
    if (!atomic_load(&running))
        return 0;
    switch (e->type) {
    case BLE_GAP_EVENT_DISC: {
        if (connecting || conn != BLE_HS_CONN_HANDLE_NONE || e->disc.rssi < -80 ||
            esp_timer_get_time() < next_attempt)
            break;
        struct ble_hs_adv_fields f;
        if (ble_hs_adv_parse_fields(&f, e->disc.data, e->disc.length_data) ||
            f.mfg_data_len != sizeof adv)
            break;
        const uint8_t *d = f.mfg_data;
        if (memcmp(d, "\xff\xffSP\x01", 5) || !sp_initiates(own_id, d + 5))
            break;
        int64_t now = esp_timer_get_time();
        bool skip = false;
        for (int i = 0; i < 16; i++)
            if (!memcmp(cooldown[i].id, d + 5, 8) && cooldown[i].until > now)
                skip = true;
        if (skip)
            break;
        memcpy(peer_id, d + 5, 8);
        connecting = true;
        central = true;
        session_started = now;
        ble_gap_disc_cancel();
        ble_gap_adv_stop();
        struct ble_gap_conn_params params = {.scan_itvl = 16,
                                             .scan_window = 16,
                                             .itvl_min = 12,
                                             .itvl_max = 24,
                                             .supervision_timeout = 400};
        if (ble_gap_connect(address_type, &e->disc.addr, 5000, &params, gap, NULL)) {
            connecting = false;
            next_attempt = now + 1000000 + (esp_random() % 1000000);
            radio_idle();
        }
        break;
    }
    case BLE_GAP_EVENT_CONNECT:
        connecting = false;
        if (e->connect.status) {
            next_attempt = esp_timer_get_time() + 2000000;
            radio_idle();
            break;
        }
        conn = e->connect.conn_handle;
        ble_gap_disc_cancel();
        ble_gap_adv_stop();
        struct ble_gap_conn_desc desc;
        if (ble_gap_conn_find(conn, &desc)) {
            end_connection();
            break;
        }
        central = desc.role == BLE_GAP_ROLE_MASTER;
        generation = (generation + 1) & 0x3fffffff;
        if (!generation)
            generation = 1;
        atomic_store(&receipt, 0);
        memset(&rx, 0, sizeof rx);
        rx_status = 0;
        submitted = false;
        success = false;
        read_mode = 1;
        read_offset = 0;
        offset = 0;
        remote_handle = 0;
        waiting = false;
        session_started = esp_timer_get_time();
        if (central) {
            phase = DISCOVER;
            if (ble_gattc_disc_chrs_by_uuid(conn, 1, 0xffff, &value_uuid.u, discovered,
                                            (void *)(uintptr_t)generation))
                end_connection();
        } else {
            phase = IDLE;
            memset(peer_id, 0, 8);
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        if (e->disconnect.conn.conn_handle != conn)
            break;
        if (peer_id[0] || peer_id[1]) {
            unsigned i = cooldown_pos++ % 16;
            memcpy(cooldown[i].id, peer_id, 8);
            cooldown[i].until = esp_timer_get_time() + (success ? 300000000LL : 10000000LL);
        }
        conn = BLE_HS_CONN_HANDLE_NONE;
        phase = IDLE;
        waiting = false;
        connecting = false;
        next_attempt = esp_timer_get_time() + 500000 + (esp_random() % 1000000);
        radio_idle();
        break;
    case BLE_GAP_EVENT_DISC_COMPLETE:
    case BLE_GAP_EVENT_ADV_COMPLETE:
        radio_idle();
        break;
    default:
        break;
    }
    return 0;
}
static void tick(struct ble_npl_event *e) {
    (void)e;
    if (!atomic_load(&running))
        return;
    uint32_t result = atomic_load(&receipt);
    if (conn != BLE_HS_CONN_HANDLE_NONE) {
        if ((result >> 2) == generation && (result & 3)) {
            rx_status = (uint8_t)(result & 3);
            if (central && phase == WAIT_LOCAL) {
                if (rx_status == 1) {
                    phase = FINISH;
                    send_next();
                } else
                    end_connection();
            }
        }
        if (central && phase == STATUS_SELECT)
            send_next();
        if (esp_timer_get_time() - session_started > 20000000)
            end_connection();
    } else
        radio_idle();
    ble_npl_callout_reset(&tick_callout, ble_npl_time_ms_to_ticks32(200));
}
static void sync_host(void) {
    if (ble_hs_util_ensure_addr(0) || ble_hs_id_infer_auto(0, &address_type)) {
        atomic_store(&ready, false);
        return;
    }
    synced = true;
    radio_idle();
    ble_npl_callout_reset(&tick_callout, ble_npl_time_ms_to_ticks32(200));
}
static void reset_host(int reason) {
    (void)reason;
    synced = false;
    atomic_store(&ready, false);
    conn = BLE_HS_CONN_HANDLE_NONE;
    connecting = false;
}
static void host(void *arg) {
    (void)arg;
    nimble_port_run();
    xSemaphoreGive(stopped);
    vTaskSuspend(NULL);
}
esp_err_t sp_ble_start(const sp_card_t *own, sp_ble_receive_fn callback) {
    if (initialized || !callback || !sp_card_valid(own))
        return ESP_ERR_INVALID_STATE;
    own_len = sp_encode(own, own_wire);
    memcpy(own_id, own->id, 8);
    receive_fn = callback;
    memcpy(adv, "\xff\xffSP\x01", 5);
    memcpy(adv + 5, own_id, 8);
    for (int i = 0; i < 4; i++)
        adv[13 + i] = (uint8_t)(own->revision >> (8 * i));
    esp_err_t e = nimble_port_init();
    if (e != ESP_OK)
        return e;
    initialized = true;
    stopped = xSemaphoreCreateBinary();
    if (!stopped) {
        nimble_port_deinit();
        initialized = false;
        return ESP_ERR_NO_MEM;
    }
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set("StreetPass");
    int rc = ble_gatts_count_cfg(services);
    if (!rc)
        rc = ble_gatts_add_svcs(services);
    if (rc) {
        vSemaphoreDelete(stopped);
        stopped = NULL;
        nimble_port_deinit();
        initialized = false;
        return ESP_FAIL;
    }
    conn = BLE_HS_CONN_HANDLE_NONE;
    connecting = false;
    synced = false;
    next_attempt = 0;
    atomic_store(&running, true);
    atomic_store(&ready, false);
    ble_hs_cfg.sync_cb = sync_host;
    ble_hs_cfg.reset_cb = reset_host;
    ble_npl_callout_init(&tick_callout, nimble_port_get_dflt_eventq(), tick, NULL);
    if (xTaskCreate(host, "sp_nimble", 6144, NULL, 5, &host_handle) != pdPASS) {
        atomic_store(&running, false);
        ble_npl_callout_deinit(&tick_callout);
        vSemaphoreDelete(stopped);
        stopped = NULL;
        nimble_port_deinit();
        initialized = false;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
esp_err_t sp_ble_stop(void) {
    if (!initialized)
        return ESP_OK;
    atomic_store(&running, false);
    atomic_store(&ready, false);
    ble_npl_callout_stop(&tick_callout);
    ble_gap_disc_cancel();
    ble_gap_adv_stop();
    if (conn != BLE_HS_CONN_HANDLE_NONE)
        ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM);
    int rc = nimble_port_stop();
    if (rc)
        return ESP_FAIL;
    xSemaphoreTake(stopped, portMAX_DELAY);
    vTaskDelete(host_handle);
    host_handle = NULL;
    ble_npl_callout_deinit(&tick_callout);
    esp_err_t e = nimble_port_deinit();
    if (e != ESP_OK)
        return e;
    vSemaphoreDelete(stopped);
    stopped = NULL;
    initialized = false;
    synced = false;
    conn = BLE_HS_CONN_HANDLE_NONE;
    return ESP_OK;
}
void sp_ble_complete(uint32_t gen, bool saved) {
    atomic_store(&receipt, (gen << 2) | (saved ? 1 : 2));
}
bool sp_ble_ready(void) {
    return atomic_load(&ready);
}
