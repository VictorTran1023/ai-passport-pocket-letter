#include "sp_screen.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    sp_screen_t screen;
    sp_screen_init(&screen, 100);
    assert(!screen.off);
    assert(sp_screen_idle(&screen, true, false, 100 + SP_SCREEN_IDLE_US - 1) ==
           SP_SCREEN_UNCHANGED);
    assert(sp_screen_idle(&screen, true, false, 100 + SP_SCREEN_IDLE_US) == SP_SCREEN_TURN_OFF);
    assert(screen.off);
    assert(sp_screen_input(&screen, false, 100 + SP_SCREEN_IDLE_US + 1) == SP_SCREEN_TURN_ON);
    assert(!screen.off);
    assert(sp_screen_input(&screen, true, 100 + SP_SCREEN_IDLE_US + 2) == SP_SCREEN_TURN_OFF);
    assert(sp_screen_idle(&screen, true, false, 100 + 2 * SP_SCREEN_IDLE_US) ==
           SP_SCREEN_UNCHANGED);
    assert(sp_screen_input(&screen, true, 100 + 2 * SP_SCREEN_IDLE_US + 1) == SP_SCREEN_TURN_ON);
    assert(sp_screen_idle(&screen, true, true, 100 + 3 * SP_SCREEN_IDLE_US + 1) ==
           SP_SCREEN_UNCHANGED);
    assert(sp_screen_idle(&screen, false, false, 100 + 3 * SP_SCREEN_IDLE_US + 1) ==
           SP_SCREEN_UNCHANGED);
    assert(sp_screen_input(&screen, false, 100 + 3 * SP_SCREEN_IDLE_US + 2) ==
           SP_SCREEN_UNCHANGED);
    assert(sp_screen_idle(&screen, true, false, 100 + 4 * SP_SCREEN_IDLE_US + 1) ==
           SP_SCREEN_UNCHANGED);
    assert(sp_screen_idle(&screen, true, false, 100 + 4 * SP_SCREEN_IDLE_US + 2) ==
           SP_SCREEN_TURN_OFF);
    puts("Pocket Letter screen state: PASS");
    return 0;
}
