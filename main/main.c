#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "sp_ble.h"
#include "sp_screen.h"
#include "sp_store.h"
#include "sp_ui.h"
#include "sp_web.h"
#include <stdio.h>
#include <string.h>
static const char *TAG = "streetpass";
typedef struct {
    bool incoming;
    bool screen_toggle;
    sp_key_t key;
    uint32_t generation;
    uint16_t length;
    uint8_t wire[SP_WIRE_MAX];
} event_t;
static QueueHandle_t events;
static sp_view_t view = {.battery = -1, .active_slot = -1, .dim = true};
static sp_nav_t before_notice;
static int notice_slot = -1;
static int64_t edit_at, notice_until, battery_at;
static bool editing, audio_ready;
static sp_screen_t screen;
static void render(void) {
    if (bsp_lvgl_lock(1000)) {
        sp_ui_render(&view);
        bsp_lvgl_unlock();
    }
}
static void index_refresh(void) {
    sp_store_index(view.index);
    int count = 0;
    for (int i = 0; i < SP_MAX_PEOPLE; i++)
        if (view.index[i].occupied) {
            int pos = count;
            while (pos > 0 && view.index[view.order[pos - 1]].seen < view.index[i].seen) {
                view.order[pos] = view.order[pos - 1];
                pos--;
            }
            view.order[pos] = i;
            count++;
        }
    view.nav.people = count;
    if (view.nav.page == SP_INBOX && view.nav.selected >= count)
        view.nav.selected = count ? count - 1 : 0;
}
static bool incoming(uint32_t gen, const uint8_t *wire, size_t n) {
    if (n > SP_WIRE_MAX)
        return false;
    event_t e = {.incoming = true, .generation = gen, .length = (uint16_t)n};
    memcpy(e.wire, wire, n);
    return xQueueSend(events, &e, 0) == pdTRUE;
}
static void on_key(bsp_btn_t button, bsp_btn_ev_t ev, void *arg) {
    (void)arg;
    event_t e = {0};
    if (ev == BSP_BTN_CLICK)
        e.key = button == BSP_BTN_UP ? SP_UP : button == BSP_BTN_DOWN ? SP_DOWN : SP_OK;
    else if (ev == BSP_BTN_LONG)
        e.key = button == BSP_BTN_UP ? SP_HOLD_UP : button == BSP_BTN_DOWN ? SP_HOLD_DOWN : SP_BACK;
    else if (ev == BSP_BTN_DOUBLE && button == BSP_BTN_OK)
        e.screen_toggle = true;
    else
        return;
    xQueueSend(events, &e, 0);
}
static void radio_start(void) {
    if (view.paused)
        return;
    sp_store_own(&view.own);
    esp_err_t e = sp_ble_start(&view.own, incoming);
    if (e != ESP_OK)
        snprintf(view.status, sizeof view.status, "蓝牙启动失败 (%x)", e);
    else
        view.status[0] = 0;
}
static void edit_stop(void) {
    sp_web_stop();
    editing = false;
    view.wifi_qr = NULL;
    view.ssid = NULL;
    sp_store_own(&view.own);
    radio_start();
}
static void card_open(int slot) {
    if (sp_store_read(slot, &view.card) == ESP_OK) {
        view.active_slot = slot;
        view.nav.own = false;
        view.nav.tab = 0;
        view.nav.page = SP_CARD;
        sp_index_t *i = &view.index[slot];
        if (sp_store_flags(slot, false, i->favorite) != ESP_OK)
            strcpy(view.status, "未读状态保存失败");
        index_refresh();
    } else {
        strcpy(view.status, "名片读取失败");
        view.nav.page = SP_HOME;
    }
}
static void chime(void) {
    if (!view.sound)
        return;
    if (!audio_ready)
        audio_ready = bsp_audio_init() == ESP_OK && bsp_audio_set_format(16000, 16, 1) == ESP_OK;
    if (!audio_ready)
        return;
    static const int16_t wave[16] = {0, 306,  566,  739,  800,  739,  566,  306,
                                     0, -306, -566, -739, -800, -739, -566, -306};
    int16_t pcm[160];
    bsp_audio_set_volume(20);
    for (int block = 0; block < 8; block++) {
        for (int i = 0; i < 160; i++)
            pcm[i] = wave[i % 16] * (block < 4 ? block + 1 : 8 - block) / 4;
        if (bsp_audio_write(pcm, sizeof pcm) != ESP_OK)
            break;
    }
    bsp_audio_set_volume(0);
}
static void action(sp_action_t a, int previous_selection) {
    if (a == SP_FAVORITE || a == SP_REMOVE) {
        int slot = view.active_slot;
        if (slot < 0 || !view.index[slot].occupied ||
            memcmp(view.index[slot].id, view.card.id, 8)) {
            strcpy(view.status, "这张名片已被更新替换");
            view.nav.page = SP_HOME;
            return;
        }
    }
    switch (a) {
    case SP_OPEN_PEER:
        if (previous_selection < view.nav.people)
            card_open(view.order[previous_selection]);
        break;
    case SP_OPEN_OWN:
        sp_store_own(&view.card);
        view.active_slot = -1;
        break;
    case SP_EDIT_START:
        strcpy(view.status, "正在开启编辑热点…");
        render();
        if (sp_ble_stop() != ESP_OK) {
            strcpy(view.status, "蓝牙未能停止，请重启");
            view.nav.page = SP_HOME;
            break;
        }
        if (sp_web_start() != ESP_OK) {
            strcpy(view.status, "热点启动失败，请重试");
            view.nav.page = SP_HOME;
            radio_start();
            break;
        }
        editing = true;
        edit_at = esp_timer_get_time();
        view.wifi_qr = sp_web_wifi_qr();
        view.ssid = sp_web_ssid();
        view.status[0] = 0;
        break;
    case SP_EDIT_STOP:
        edit_stop();
        break;
    case SP_FAVORITE:
        if (view.active_slot >= 0) {
            sp_index_t *i = &view.index[view.active_slot];
            if (sp_store_flags(view.active_slot, false, !i->favorite) != ESP_OK)
                strcpy(view.status, "收藏保存失败");
            index_refresh();
            view.nav.tab = 2;
        }
        break;
    case SP_REMOVE:
        if (sp_store_delete(view.active_slot) != ESP_OK) {
            strcpy(view.status, "删除失败，请重试");
            view.nav.page = SP_HOME;
        }
        index_refresh();
        break;
    case SP_PAUSE:
        if (view.paused) {
            view.paused = false;
            radio_start();
        } else if (sp_ble_stop() == ESP_OK)
            view.paused = true;
        else
            strcpy(view.status, "暂停失败，请重启");
        break;
    case SP_SOUND:
        if (!view.sound && !audio_ready) {
            audio_ready =
                bsp_audio_init() == ESP_OK && bsp_audio_set_format(16000, 16, 1) == ESP_OK;
            bsp_audio_set_volume(0);
        }
        if (audio_ready)
            view.sound = !view.sound;
        else
            strcpy(view.status, "提示音暂不可用");
        break;
    case SP_DIM:
        view.dim = !view.dim;
        break;
    default:
        break;
    }
    if (a == SP_PAUSE || a == SP_SOUND || a == SP_DIM) {
        uint8_t bits = (view.paused ? 1 : 0) | (view.sound ? 2 : 0) | (view.dim ? 4 : 0);
        if (sp_store_set_settings(bits) != ESP_OK)
            strcpy(view.status, "设置保存失败，请重试");
    }
}
static void worker(void *arg) {
    (void)arg;
    sp_screen_init(&screen, esp_timer_get_time());
    radio_start();
    index_refresh();
    render();
    for (;;) {
        event_t e;
        bool redraw = false;
        if (xQueueReceive(events, &e, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (e.incoming) {
                sp_card_t card;
                int slot = -1;
                bool changed = false;
                bool saved = sp_decode(&card, e.wire, e.length) &&
                             sp_store_receive(&card, &slot, &changed) == ESP_OK;
                sp_ble_complete(e.generation, saved);
                if (saved) {
                    index_refresh();
                    if (changed && !editing) {
                        if (view.nav.page != SP_NOTICE)
                            before_notice = view.nav;
                        strcpy(view.notice, card.text[0]);
                        notice_slot = slot;
                        view.nav.page = SP_NOTICE;
                        notice_until = esp_timer_get_time() + 4000000;
                        chime();
                        redraw = true;
                    }
                } else {
                    strcpy(view.status, "名片未保存，请检查空间");
                    redraw = true;
                }
            } else {
                sp_screen_change_t change =
                    sp_screen_input(&screen, e.screen_toggle, esp_timer_get_time());
                if (change != SP_SCREEN_UNCHANGED) {
                    bsp_display_backlight(change == SP_SCREEN_TURN_ON ? 35 : 0);
                    continue;
                }
                if (view.nav.page == SP_NOTICE) {
                    view.nav = before_notice;
                    index_refresh();
                    if (e.key == SP_OK)
                        card_open(notice_slot);
                } else {
                    int old = view.nav.selected;
                    sp_action_t a = sp_nav_key(&view.nav, e.key);
                    action(a, old);
                }
                redraw = true;
            }
        }
        int64_t now = esp_timer_get_time();
        if (view.nav.page == SP_NOTICE && now >= notice_until) {
            view.nav = before_notice;
            index_refresh();
            redraw = true;
        }
        if (editing && now - edit_at > 600000000LL) {
            edit_stop();
            view.nav.page = SP_HOME;
            strcpy(view.status, "编辑已超时，热点已关闭");
            redraw = true;
        }
        bool ready = sp_ble_ready();
        if (ready != view.radio_ready) {
            view.radio_ready = ready;
            redraw = true;
        }
        if (now >= battery_at) {
            view.battery = bsp_battery_soc();
            battery_at = now + 30000000;
            redraw = true;
        }
        if (sp_screen_idle(&screen, view.dim, editing, now) == SP_SCREEN_TURN_OFF)
            bsp_display_backlight(0);
        if (redraw)
            render();
    }
}
void app_main(void) {
    bsp_i2c_init();
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "Display initialization failed");
        return;
    }
    bsp_display_backlight(35);
    esp_err_t e = nvs_flash_init();
    if (e == ESP_OK)
        e = sp_store_init();
    if (e != ESP_OK) {
        view.nav.page = SP_ERROR;
        snprintf(view.status, sizeof view.status, "存储初始化失败 (%x)\n数据未被清空", e);
        render();
        return;
    }
    sp_store_own(&view.own);
    uint8_t settings = 4;
    if (sp_store_get_settings(&settings) == ESP_OK) {
        view.paused = (settings & 1) != 0;
        view.sound = (settings & 2) != 0;
        view.dim = (settings & 4) != 0;
    } else {
        view.paused = true;
        strcpy(view.status, "设置读取失败，已暂停");
    }
    bsp_battery_init();
    view.battery = bsp_battery_soc();
    events = xQueueCreate(6, sizeof(event_t));
    if (!events) {
        view.nav.page = SP_ERROR;
        strcpy(view.status, "内存不足");
        render();
        return;
    }
    if (bsp_button_init(on_key, NULL) != ESP_OK) {
        view.nav.page = SP_ERROR;
        strcpy(view.status, "按键初始化失败");
        render();
        return;
    }
    if (xTaskCreate(worker, "streetpass", 8192, NULL, 4, NULL) != pdPASS) {
        view.nav.page = SP_ERROR;
        strcpy(view.status, "应用任务启动失败");
        render();
        return;
    }
    ESP_LOGI(TAG, "Pocket Letter started; free heap: %lu", (unsigned long)esp_get_free_heap_size());
}
