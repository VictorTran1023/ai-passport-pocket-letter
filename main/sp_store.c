#include "sp_store.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
static SemaphoreHandle_t mutex;
static nvs_handle_t db;
static sp_index_t index_cache[SP_MAX_PEOPLE];
static sp_card_t own;
static uint32_t sequence;
/* A record is a versioned wire card prefixed by local flags and monotonic encounter order. */
static void key_for(int slot, char key[8]) {
    snprintf(key, 8, "c%03d", slot);
}
static esp_err_t load(int slot, sp_card_t *card, uint8_t *flags, uint32_t *seen) {
    char key[8];
    key_for(slot, key);
    uint8_t data[SP_WIRE_MAX + 5];
    size_t len = sizeof data;
    esp_err_t e = nvs_get_blob(db, key, data, &len);
    if (e != ESP_OK)
        return e;
    if (len < 5 || !sp_decode(card, data + 5, len - 5))
        return ESP_ERR_INVALID_CRC;
    *flags = data[0];
    *seen = (uint32_t)data[1] | (uint32_t)data[2] << 8 | (uint32_t)data[3] << 16 |
            (uint32_t)data[4] << 24;
    return ESP_OK;
}
static void cache(int slot, const sp_card_t *card, uint8_t flags, uint32_t seen) {
    sp_index_t *i = &index_cache[slot];
    memset(i, 0, sizeof *i);
    i->occupied = true;
    memcpy(i->id, card->id, 8);
    i->revision = card->revision;
    i->seen = seen;
    i->favorite = (flags & 2) != 0;
    i->unread = (flags & 1) != 0;
    memcpy(i->name, card->text[0], sizeof i->name);
}
static esp_err_t save(int slot, const sp_card_t *card, uint8_t flags, uint32_t seen) {
    uint8_t data[SP_WIRE_MAX + 5];
    size_t len = sp_encode(card, data + 5);
    if (!len)
        return ESP_ERR_INVALID_ARG;
    data[0] = flags;
    for (int j = 0; j < 4; j++)
        data[j + 1] = (uint8_t)(seen >> (j * 8));
    char key[8];
    key_for(slot, key);
    esp_err_t e = nvs_set_blob(db, key, data, len + 5);
    if (e == ESP_OK)
        e = nvs_commit(db);
    if (e == ESP_OK)
        cache(slot, card, flags, seen);
    return e;
}
esp_err_t sp_store_init(void) {
    mutex = xSemaphoreCreateMutex();
    if (!mutex)
        return ESP_ERR_NO_MEM;
    esp_err_t e = nvs_flash_init_partition("spdata");
    if (e != ESP_OK)
        return e;
    e = nvs_open_from_partition("spdata", "streetpass", NVS_READWRITE, &db);
    if (e != ESP_OK)
        return e;
    uint8_t data[SP_WIRE_MAX];
    size_t n = sizeof data;
    e = nvs_get_blob(db, "own", data, &n);
    if (e == ESP_ERR_NVS_NOT_FOUND) {
        esp_fill_random(own.id, 8);
        own.id[0] |= 1;
        own.revision = 1;
        strcpy(own.text[0], "新朋友");
        n = sp_encode(&own, data);
        e = nvs_set_blob(db, "own", data, n);
        if (e == ESP_OK)
            e = nvs_commit(db);
    } else if (e == ESP_OK && !sp_decode(&own, data, n))
        e = ESP_ERR_INVALID_CRC;
    if (e != ESP_OK)
        return e;
    for (int j = 0; j < SP_MAX_PEOPLE; j++) {
        sp_card_t card;
        uint8_t flags;
        uint32_t seen;
        e = load(j, &card, &flags, &seen);
        if (e == ESP_ERR_NVS_NOT_FOUND)
            continue;
        if (e != ESP_OK)
            return e; /* Preserve unreadable data rather than silently deleting it. */
        cache(j, &card, flags, seen);
        if (seen > sequence)
            sequence = seen;
    }
    return ESP_OK;
}
esp_err_t sp_store_own(sp_card_t *out) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    *out = own;
    xSemaphoreGive(mutex);
    return ESP_OK;
}
esp_err_t sp_store_save_own(const sp_card_t *card) {
    if (!sp_card_valid(card))
        return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(mutex, portMAX_DELAY);
    esp_err_t e = ESP_ERR_INVALID_STATE;
    if (!memcmp(card->id, own.id, 8) && own.revision < UINT32_MAX &&
        card->revision == own.revision + 1) {
        uint8_t data[SP_WIRE_MAX];
        size_t n = sp_encode(card, data);
        e = nvs_set_blob(db, "own", data, n);
        if (e == ESP_OK)
            e = nvs_commit(db);
        if (e == ESP_OK)
            own = *card;
    }
    xSemaphoreGive(mutex);
    return e;
}
esp_err_t sp_store_receive(const sp_card_t *card, int *slot, bool *changed) {
    if (!sp_card_valid(card))
        return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(mutex, portMAX_DELAY);
    esp_err_t e = ESP_ERR_INVALID_ARG;
    *changed = false;
    if (!memcmp(card->id, own.id, 8))
        goto done;
    *slot = sp_slot(index_cache, SP_MAX_PEOPLE, card->id);
    if (*slot < 0) {
        e = ESP_ERR_NO_MEM;
        goto done;
    }
    sp_index_t *i = &index_cache[*slot];
    bool same = i->occupied && !memcmp(i->id, card->id, 8);
    *changed = !same || card->revision > i->revision;
    if (same && card->revision <= i->revision) {
        e = ESP_OK;
        goto done;
    }
    if (sequence == UINT32_MAX) {
        e = ESP_ERR_INVALID_STATE;
        goto done;
    }
    e = save(*slot, card, (same && i->favorite ? 2 : 0) | 1, sequence + 1);
    if (e == ESP_OK)
        sequence++;
done:
    xSemaphoreGive(mutex);
    return e;
}
esp_err_t sp_store_read(int slot, sp_card_t *out) {
    if (slot < 0 || slot >= SP_MAX_PEOPLE)
        return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(mutex, portMAX_DELAY);
    uint8_t flags;
    uint32_t seen;
    esp_err_t e = load(slot, out, &flags, &seen);
    xSemaphoreGive(mutex);
    return e;
}
esp_err_t sp_store_flags(int slot, bool unread, bool favorite) {
    if (slot < 0 || slot >= SP_MAX_PEOPLE)
        return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(mutex, portMAX_DELAY);
    sp_card_t card;
    uint8_t flags;
    uint32_t seen;
    esp_err_t e = load(slot, &card, &flags, &seen);
    if (e == ESP_OK)
        e = save(slot, &card, (unread ? 1 : 0) | (favorite ? 2 : 0), seen);
    xSemaphoreGive(mutex);
    return e;
}
esp_err_t sp_store_delete(int slot) {
    if (slot < 0 || slot >= SP_MAX_PEOPLE)
        return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(mutex, portMAX_DELAY);
    char key[8];
    key_for(slot, key);
    esp_err_t e = nvs_erase_key(db, key);
    if (e == ESP_OK)
        e = nvs_commit(db);
    if (e == ESP_OK)
        memset(&index_cache[slot], 0, sizeof index_cache[slot]);
    xSemaphoreGive(mutex);
    return e;
}
void sp_store_index(sp_index_t out[SP_MAX_PEOPLE]) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    memcpy(out, index_cache, sizeof index_cache);
    xSemaphoreGive(mutex);
}

esp_err_t sp_store_get_settings(uint8_t *bits) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    *bits = 4;
    esp_err_t e = nvs_get_u8(db, "settings", bits);
    xSemaphoreGive(mutex);
    if (e == ESP_OK && (*bits & ~7))
        return ESP_ERR_INVALID_ARG;
    return e == ESP_ERR_NVS_NOT_FOUND ? ESP_OK : e;
}
esp_err_t sp_store_set_settings(uint8_t bits) {
    if (bits & ~7)
        return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(mutex, portMAX_DELAY);
    esp_err_t e = nvs_set_u8(db, "settings", bits);
    if (e == ESP_OK)
        e = nvs_commit(db);
    xSemaphoreGive(mutex);
    return e;
}
