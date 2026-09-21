#include "sp_nav.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    sp_nav_t n = {.page = SP_HOME, .selected = 2};
    assert(sp_nav_key(&n, SP_OK) == SP_EDIT_START);
    assert(n.page == SP_EDIT_WIFI);
    assert(sp_nav_key(&n, SP_OK) == SP_NONE && n.page == SP_EDIT_URL);
    assert(sp_nav_key(&n, SP_BACK) == SP_EDIT_STOP && n.page == SP_HOME);
    n = (sp_nav_t){.page = SP_INBOX, .people = 0};
    assert(sp_nav_key(&n, SP_OK) == SP_NONE);
    sp_nav_key(&n, SP_DOWN);
    assert(n.selected == 0);
    n.people = 100;
    sp_nav_key(&n, SP_UP);
    assert(n.selected == 99);
    assert(sp_nav_key(&n, SP_OK) == SP_OPEN_PEER && n.page == SP_CARD);
    n.own = false;
    sp_nav_key(&n, SP_HOLD_DOWN);
    assert(n.page == SP_DELETE && n.selected == 0);
    assert(sp_nav_key(&n, SP_OK) == SP_NONE && n.page == SP_CARD);
    sp_nav_key(&n, SP_HOLD_DOWN);
    sp_nav_key(&n, SP_DOWN);
    assert(sp_nav_key(&n, SP_OK) == SP_REMOVE && n.page == SP_INBOX);
    n = (sp_nav_t){.page = SP_CARD, .own = true};
    assert(sp_nav_key(&n, SP_HOLD_UP) == SP_NONE);
    sp_nav_key(&n, SP_HOLD_DOWN);
    assert(n.page == SP_CARD);
    puts("StreetPass navigation: PASS");
    return 0;
}
