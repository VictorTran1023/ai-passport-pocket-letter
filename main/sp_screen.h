#pragma once
#include <stdbool.h>
#include <stdint.h>

#define SP_SCREEN_IDLE_US 30000000LL

typedef enum {
    SP_SCREEN_UNCHANGED,
    SP_SCREEN_TURN_OFF,
    SP_SCREEN_TURN_ON,
} sp_screen_change_t;

typedef struct {
    bool off;
    int64_t last_input_us;
} sp_screen_t;

void sp_screen_init(sp_screen_t *screen, int64_t now_us);
sp_screen_change_t sp_screen_input(sp_screen_t *screen, bool double_ok, int64_t now_us);
sp_screen_change_t sp_screen_idle(sp_screen_t *screen, bool auto_off, bool editing,
                                  int64_t now_us);
