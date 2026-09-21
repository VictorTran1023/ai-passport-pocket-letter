#include "fake_idf.h"
#include "sp_store.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static struct {
    char key[16];
    uint8_t data[SP_WIRE_MAX + 5];
    size_t size;
} db[102];
static bool locked, fail_write;
static int commits;
static int find(const char *key, bool create) {
    int empty = -1;
    for (int i = 0; i < 102; i++) {
        if (!strcmp(db[i].key, key))
            return i;
        if (!*db[i].key && empty < 0)
            empty = i;
    }
    if (create && empty >= 0) {
        strcpy(db[empty].key, key);
        return empty;
    }
    return -1;
}
SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    return &locked;
}
int xSemaphoreTake(SemaphoreHandle_t s, uint32_t t) {
    (void)s;
    (void)t;
    assert(!locked);
    locked = true;
    return 1;
}
int xSemaphoreGive(SemaphoreHandle_t s) {
    (void)s;
    assert(locked);
    locked = false;
    return 1;
}
void esp_fill_random(void *p, size_t n) {
    memset(p, 0xaa, n);
}
esp_err_t nvs_flash_init_partition(const char *p) {
    assert(!strcmp(p, "spdata"));
    return ESP_OK;
}
esp_err_t nvs_open_from_partition(const char *p, const char *ns, int mode, nvs_handle_t *h) {
    assert(!strcmp(p, "spdata") && !strcmp(ns, "streetpass") && mode == NVS_READWRITE);
    *h = 1;
    return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *data, size_t *n) {
    (void)h;
    int i = find(key, false);
    if (i < 0)
        return ESP_ERR_NVS_NOT_FOUND;
    assert(*n >= db[i].size);
    *n = db[i].size;
    memcpy(data, db[i].data, *n);
    return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *data, size_t n) {
    (void)h;
    if (fail_write) {
        fail_write = false;
        return ESP_FAIL;
    }
    int i = find(key, true);
    assert(i >= 0 && n <= sizeof db[i].data);
    memcpy(db[i].data, data, n);
    db[i].size = n;
    return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h) {
    (void)h;
    commits++;
    return ESP_OK;
}
esp_err_t nvs_erase_key(nvs_handle_t h, const char *key) {
    (void)h;
    int i = find(key, false);
    if (i < 0)
        return ESP_ERR_NVS_NOT_FOUND;
    memset(&db[i], 0, sizeof db[i]);
    return ESP_OK;
}
esp_err_t nvs_get_u8(nvs_handle_t h, const char *key, uint8_t *v) {
    size_t n = 1;
    return nvs_get_blob(h, key, v, &n);
}
esp_err_t nvs_set_u8(nvs_handle_t h, const char *key, uint8_t v) {
    return nvs_set_blob(h, key, &v, 1);
}
static sp_card_t peer(int number) {
    sp_card_t c = {.revision = 1};
    c.id[0] = (uint8_t)number;
    c.id[1] = (uint8_t)(number >> 8);
    snprintf(c.text[0], SP_TEXT_CAP, "Peer %d", number);
    return c;
}
int main(void) {
    assert(sp_store_init() == ESP_OK);
    sp_card_t own, read;
    assert(sp_store_own(&own) == ESP_OK);
    assert(sp_card_valid(&own));
    uint8_t bits = 0;
    assert(sp_store_get_settings(&bits) == ESP_OK && bits == 4);
    assert(sp_store_set_settings(7) == ESP_OK);
    assert(sp_store_get_settings(&bits) == ESP_OK && bits == 7);
    fail_write = true;
    assert(sp_store_set_settings(0) != ESP_OK);
    assert(sp_store_get_settings(&bits) == ESP_OK && bits == 7);
    sp_card_t update = own;
    update.revision++;
    strcpy(update.text[0], "Updated");
    fail_write = true;
    assert(sp_store_save_own(&update) != ESP_OK);
    sp_store_own(&read);
    assert(!memcmp(&read, &own, sizeof own));
    assert(sp_store_save_own(&update) == ESP_OK);
    assert(sp_store_save_own(&update) == ESP_ERR_INVALID_STATE);
    int slot;
    bool changed;
    assert(sp_store_receive(&own, &slot, &changed) == ESP_ERR_INVALID_ARG);
    for (int i = 1; i <= 100; i++) {
        sp_card_t c = peer(i);
        assert(sp_store_receive(&c, &slot, &changed) == ESP_OK && changed && slot == i - 1);
    }
    sp_card_t c = peer(1);
    int previous = commits;
    assert(sp_store_receive(&c, &slot, &changed) == ESP_OK && !changed && slot == 0 &&
           commits == previous);
    assert(sp_store_flags(0, false, true) == ESP_OK);
    c = peer(101);
    assert(sp_store_receive(&c, &slot, &changed) == ESP_OK && slot == 1);
    assert(sp_store_read(0, &read) == ESP_OK && read.id[0] == 1);
    for (int i = 0; i < 100; i++)
        assert(sp_store_flags(i, false, true) == ESP_OK);
    c = peer(102);
    assert(sp_store_receive(&c, &slot, &changed) == ESP_ERR_NO_MEM);
    c = peer(1);
    c.revision = 2;
    strcpy(c.text[2], "A new message");
    assert(sp_store_receive(&c, &slot, &changed) == ESP_OK && slot == 0 && changed);
    sp_index_t ix[SP_MAX_PEOPLE];
    sp_store_index(ix);
    assert(ix[0].favorite && ix[0].unread && ix[0].revision == 2);
    c.revision = 1;
    assert(sp_store_receive(&c, &slot, &changed) == ESP_OK && !changed);
    assert(sp_store_read(0, &read) == ESP_OK && read.revision == 2);
    assert(sp_store_delete(0) == ESP_OK);
    sp_store_index(ix);
    assert(!ix[0].occupied);
    c = peer(103);
    assert(sp_store_receive(&c, &slot, &changed) == ESP_OK && slot == 0);
    puts("StreetPass storage: PASS (fake NVS; physical power-loss behavior requires hardware)");
    return 0;
}
