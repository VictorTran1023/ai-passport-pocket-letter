#pragma once
#include "sp_core.h"
#include "sp_nav.h"
typedef struct {
    sp_nav_t nav;
    sp_card_t card, own;
    sp_index_t index[SP_MAX_PEOPLE];
    int order[SP_MAX_PEOPLE];
    int active_slot, battery;
    bool paused, sound, dim, radio_ready;
    char status[96], notice[49];
    const char *wifi_qr, *ssid;
} sp_view_t;
/* All calls require the BSP LVGL lock. No storage or networking occurs here. */
void sp_ui_render(const sp_view_t *view);
