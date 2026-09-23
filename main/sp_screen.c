#include "sp_screen.h"

void sp_screen_init(sp_screen_t *screen, int64_t now_us) {
    screen->off = false;
    screen->last_input_us = now_us;
}

sp_screen_change_t sp_screen_input(sp_screen_t *screen, bool double_ok, int64_t now_us) {
    screen->last_input_us = now_us;
    if (screen->off) {
        screen->off = false;
        return SP_SCREEN_TURN_ON;
    }
    if (double_ok) {
        screen->off = true;
        return SP_SCREEN_TURN_OFF;
    }
    return SP_SCREEN_UNCHANGED;
}

sp_screen_change_t sp_screen_idle(sp_screen_t *screen, bool auto_off, bool editing,
                                  int64_t now_us) {
    if (!auto_off || editing || screen->off || now_us < screen->last_input_us ||
        now_us - screen->last_input_us < SP_SCREEN_IDLE_US)
        return SP_SCREEN_UNCHANGED;
    screen->off = true;
    return SP_SCREEN_TURN_OFF;
}
