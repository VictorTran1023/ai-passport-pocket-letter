#pragma once
#include "esp_err.h"
#include "sp_core.h"
/* NVS access is serialized internally. Call from worker tasks, never LVGL/button callbacks. */
esp_err_t sp_store_init(void);
esp_err_t sp_store_own(sp_card_t *out);
esp_err_t sp_store_save_own(const sp_card_t *card);
esp_err_t sp_store_receive(const sp_card_t *card, int *slot, bool *changed);
esp_err_t sp_store_read(int slot, sp_card_t *out);
esp_err_t sp_store_flags(int slot, bool unread, bool favorite);
esp_err_t sp_store_delete(int slot);
void sp_store_index(sp_index_t out[SP_MAX_PEOPLE]);

/* Bits: paused=1, sound=2, idle dimming=4. Defaults to dimming enabled. */
esp_err_t sp_store_get_settings(uint8_t *bits);
esp_err_t sp_store_set_settings(uint8_t bits);
