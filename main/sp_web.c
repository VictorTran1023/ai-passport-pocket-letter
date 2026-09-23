#include "sp_web.h"
#include "cJSON.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "sp_store.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
static httpd_handle_t server;
static esp_netif_t *netif;
static bool wifi_initialized;
static atomic_bool dns_running;
static SemaphoreHandle_t dns_done;
static char ssid[32], password[17], token[33], qr[128];
extern const char editor_start[] asm("_binary_sp_editor_html_start");
extern const char editor_end[] asm("_binary_sp_editor_html_end");
static void random_hex(char *out, size_t bytes) {
    const char *hex = "0123456789abcdef";
    for (size_t i = 0; i < bytes; i++) {
        uint8_t n = (uint8_t)esp_random();
        out[i * 2] = hex[n >> 4];
        out[i * 2 + 1] = hex[n & 15];
    }
    out[bytes * 2] = 0;
}
static void headers(httpd_req_t *r, const char *type) {
    httpd_resp_set_type(r, type);
    httpd_resp_set_hdr(r, "Cache-Control", "no-store");
    httpd_resp_set_hdr(r, "X-Content-Type-Options", "nosniff");
}
static esp_err_t redirect(httpd_req_t *r);
static esp_err_t home(httpd_req_t *r) {
    char requested_host[40];
    if (httpd_req_get_hdr_value_str(r, "Host", requested_host, sizeof requested_host) != ESP_OK ||
        (strcmp(requested_host, "192.168.4.1") && strcmp(requested_host, "192.168.4.1:80")))
        return redirect(r);
    headers(r, "text/html; charset=utf-8");
    httpd_resp_set_hdr(
        r, "Content-Security-Policy",
        "default-src 'self'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src "
        "'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
    return httpd_resp_send(r, editor_start, editor_end - editor_start - 1);
}
static esp_err_t get_card(httpd_req_t *r) {
    sp_card_t card;
    sp_store_own(&card);
    cJSON *j = cJSON_CreateObject();
    if (!j)
        return ESP_ERR_NO_MEM;
    cJSON_AddStringToObject(j, "token", token);
    cJSON_AddNumberToObject(j, "revision", card.revision);
    cJSON *arr = cJSON_AddArrayToObject(j, "fields");
    for (int i = 0; i < SP_FIELD_COUNT; i++)
        cJSON_AddItemToArray(arr, cJSON_CreateString(card.text[i]));
    char *body = cJSON_PrintUnformatted(j);
    cJSON_Delete(j);
    if (!body)
        return ESP_ERR_NO_MEM;
    headers(r, "application/json");
    esp_err_t e = httpd_resp_sendstr(r, body);
    cJSON_free(body);
    return e;
}
static esp_err_t bad(httpd_req_t *r, const char *status, const char *msg) {
    headers(r, "text/plain; charset=utf-8");
    httpd_resp_set_status(r, status);
    return httpd_resp_sendstr(r, msg);
}
static esp_err_t save_card(httpd_req_t *r) {
    char supplied[40], origin[80];
    if (httpd_req_get_hdr_value_str(r, "X-StreetPass-Token", supplied, sizeof supplied) != ESP_OK ||
        strcmp(supplied, token))
        return bad(r, "403 Forbidden", "请重新打开编辑页");
    if (httpd_req_get_hdr_value_len(r, "Origin") &&
        (httpd_req_get_hdr_value_str(r, "Origin", origin, sizeof origin) != ESP_OK ||
         strcmp(origin, "http://192.168.4.1")))
        return bad(r, "403 Forbidden", "请使用设备本地页面");
    if (r->content_len < 2 || r->content_len > 2048)
        return bad(r, "413 Content Too Large", "名片太长");
    char body[2049];
    size_t used = 0;
    while (used < r->content_len) {
        int n = httpd_req_recv(r, body + used, r->content_len - used);
        if (n <= 0)
            return ESP_FAIL;
        used += (size_t)n;
    }
    body[used] = 0;
    if (memchr(body, 0, used) || strstr(body, "\\u0000"))
        return bad(r, "400 Bad Request", "不支持的字符");
    const char *end = NULL;
    cJSON *j = cJSON_ParseWithLengthOpts(body, used + 1, &end, true);
    if (!j)
        return bad(r, "400 Bad Request", "名片格式错误");
    sp_card_t card;
    sp_store_own(&card);
    cJSON *rev = cJSON_GetObjectItemCaseSensitive(j, "revision"),
          *fields = cJSON_GetObjectItemCaseSensitive(j, "fields");
    bool valid = cJSON_IsNumber(rev) && rev->valuedouble == card.revision &&
                 card.revision < UINT32_MAX && cJSON_IsArray(fields) &&
                 cJSON_GetArraySize(fields) == SP_FIELD_COUNT;
    for (int i = 0; valid && i < SP_FIELD_COUNT; i++) {
        cJSON *v = cJSON_GetArrayItem(fields, i);
        valid = cJSON_IsString(v) && strlen(v->valuestring) <= sp_limits[i];
        if (valid) {
            memset(card.text[i], 0, SP_TEXT_CAP);
            strcpy(card.text[i], v->valuestring);
        }
    }
    cJSON_Delete(j);
    if (!valid)
        return bad(r, "409 Conflict", "内容无效或版本已更新，请刷新后重试");
    card.revision++;
    if (!sp_card_valid(&card))
        return bad(r, "400 Bad Request", "请检查昵称、字数和特殊字符");
    if (sp_store_save_own(&card) != ESP_OK)
        return bad(r, "503 Service Unavailable", "设备保存失败，请重试");
    char reply[64];
    snprintf(reply, sizeof reply, "{\"revision\":%lu}", (unsigned long)card.revision);
    headers(r, "application/json");
    return httpd_resp_sendstr(r, reply);
}
static esp_err_t redirect(httpd_req_t *r) {
    httpd_resp_set_status(r, "302 Found");
    httpd_resp_set_hdr(r, "Location", "http://192.168.4.1/");
    return httpd_resp_sendstr(r, "");
}
/* Bounded single-question DNS A reply. Reject compression, oversized labels and nonqueries. */
static void dns_task(void *arg) {
    int fd = (int)(intptr_t)arg;
    uint8_t packet[512];
    while (atomic_load(&dns_running)) {
        struct sockaddr_in from;
        socklen_t flen = sizeof from;
        int n = recvfrom(fd, packet, sizeof packet - 16, 0, (struct sockaddr *)&from, &flen);
        if (n < 17 || packet[2] & 0xf8 || packet[4] != 0 || packet[5] != 1)
            continue;
        size_t pos = 12;
        bool valid = true;
        while (pos < (size_t)n && packet[pos]) {
            size_t label = packet[pos++];
            if (label > 63 || pos + label >= (size_t)n) {
                valid = false;
                break;
            }
            pos += label;
        }
        if (!valid || pos + 5 > (size_t)n)
            continue;
        pos++;
        bool a = packet[pos] == 0 && packet[pos + 1] == 1 && packet[pos + 2] == 0 &&
                 packet[pos + 3] == 1;
        pos += 4;
        packet[2] = 0x81;
        packet[3] = 0x80;
        packet[6] = 0;
        packet[7] = a ? 1 : 0;
        memset(packet + 8, 0, 4);
        if (a) {
            const uint8_t answer[] = {0xc0, 0x0c, 0, 1, 0, 1, 0, 0, 0, 10, 0, 4, 192, 168, 4, 1};
            memcpy(packet + pos, answer, sizeof answer);
            pos += sizeof answer;
        }
        sendto(fd, packet, pos, 0, (struct sockaddr *)&from, flen);
    }
    close(fd);
    xSemaphoreGive(dns_done);
    vTaskDelete(NULL);
}
esp_err_t sp_web_start(void) {
    if (server || wifi_initialized)
        return ESP_ERR_INVALID_STATE;
    char suffix[9];
    random_hex(suffix, 4);
    snprintf(ssid, sizeof ssid, "PocketLetter-%s", suffix);
    random_hex(password, 8);
    random_hex(token, 16);
    snprintf(qr, sizeof qr, "WIFI:T:WPA;S:%s;P:%s;;", ssid, password);
    esp_err_t e = esp_netif_init();
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE)
        return e;
    e = esp_event_loop_create_default();
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE)
        return e;
    netif = esp_netif_create_default_wifi_ap();
    if (!netif)
        return ESP_ERR_NO_MEM;
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    e = esp_wifi_init(&init);
    if (e != ESP_OK)
        goto fail;
    wifi_initialized = true;
    wifi_config_t config = {0};
    strcpy((char *)config.ap.ssid, ssid);
    strcpy((char *)config.ap.password, password);
    config.ap.ssid_len = strlen(ssid);
    config.ap.channel = 6;
    config.ap.max_connection = 1;
    config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    e = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (e != ESP_OK)
        goto fail;
    e = esp_wifi_set_mode(WIFI_MODE_AP);
    if (e != ESP_OK)
        goto fail;
    e = esp_wifi_set_config(WIFI_IF_AP, &config);
    if (e != ESP_OK)
        goto fail;
    esp_netif_dns_info_t info = {0};
    info.ip.type = ESP_IPADDR_TYPE_V4;
    info.ip.u_addr.ip4.addr = ESP_IP4TOADDR(192, 168, 4, 1);
    esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &info);
    uint8_t offer = 1;
    esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &offer,
                           sizeof offer);
    e = esp_wifi_start();
    if (e != ESP_OK)
        goto fail;
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size = 8192;
    cfg.max_open_sockets = 3;
    cfg.lru_purge_enable = true;
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    cfg.recv_wait_timeout = 3;
    cfg.send_wait_timeout = 3;
    e = httpd_start(&server, &cfg);
    if (e != ESP_OK)
        goto fail;
    const httpd_uri_t uris[] = {{.uri = "/", .method = HTTP_GET, .handler = home},
                                {.uri = "/api/card", .method = HTTP_GET, .handler = get_card},
                                {.uri = "/api/card", .method = HTTP_POST, .handler = save_card},
                                {.uri = "/*", .method = HTTP_GET, .handler = redirect}};
    for (size_t i = 0; i < sizeof uris / sizeof uris[0]; i++) {
        e = httpd_register_uri_handler(server, &uris[i]);
        if (e != ESP_OK)
            goto fail;
    }
    dns_done = xSemaphoreCreateBinary();
    if (!dns_done) {
        e = ESP_ERR_NO_MEM;
        goto fail;
    }
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        e = ESP_FAIL;
        goto fail;
    }
    struct sockaddr_in addr = {
        .sin_family = AF_INET, .sin_port = htons(53), .sin_addr.s_addr = htonl(INADDR_ANY)};
    struct timeval timeout = {.tv_sec = 0, .tv_usec = 250000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        close(fd);
        e = ESP_FAIL;
        goto fail;
    }
    atomic_store(&dns_running, true);
    if (xTaskCreate(dns_task, "sp_dns", 3072, (void *)(intptr_t)fd, 3, NULL) != pdPASS) {
        atomic_store(&dns_running, false);
        close(fd);
        e = ESP_ERR_NO_MEM;
        goto fail;
    }
    return ESP_OK;
fail:
    sp_web_stop();
    return e;
}
void sp_web_stop(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    if (atomic_exchange(&dns_running, false))
        xSemaphoreTake(dns_done, portMAX_DELAY);
    if (dns_done) {
        vSemaphoreDelete(dns_done);
        dns_done = NULL;
    }
    if (wifi_initialized) {
        esp_wifi_stop();
        esp_wifi_deinit();
        wifi_initialized = false;
    }
    if (netif) {
        esp_netif_destroy_default_wifi(netif);
        netif = NULL;
    }
    memset(password, 0, sizeof password);
    memset(token, 0, sizeof token);
    memset(qr, 0, sizeof qr);
}
const char *sp_web_wifi_qr(void) { return qr; }
const char *sp_web_ssid(void) { return ssid; }
