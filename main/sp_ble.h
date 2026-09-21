#pragma once
#include "esp_err.h"
#include "sp_core.h"
/* Callback executes on NimBLE's task: copy into a bounded worker queue and return immediately. */
typedef bool (*sp_ble_receive_fn)(uint32_t generation, const uint8_t *wire, size_t length);
esp_err_t sp_ble_start(const sp_card_t *own, sp_ble_receive_fn receive);
esp_err_t sp_ble_stop(void);
void sp_ble_complete(uint32_t generation, bool saved);
bool sp_ble_ready(void);
