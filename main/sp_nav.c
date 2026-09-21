#include "sp_nav.h"
static void page(sp_nav_t *n, sp_page_t p) {
    n->page = p;
    n->selected = 0;
}
sp_action_t sp_nav_key(sp_nav_t *n, sp_key_t k) {
    if (k == SP_BACK) {
        if (n->page == SP_EDIT_WIFI || n->page == SP_EDIT_URL) {
            page(n, SP_HOME);
            return SP_EDIT_STOP;
        }
        if (n->page == SP_QR)
            page(n, SP_CONTACTS);
        else if (n->page == SP_CONTACTS || n->page == SP_DELETE)
            page(n, SP_CARD);
        else if (n->page == SP_CARD)
            page(n, n->own ? SP_OWN : SP_INBOX);
        else
            page(n, SP_HOME);
        return SP_NONE;
    }
    if (n->page == SP_CARD) {
        if (!n->own && k == SP_HOLD_UP)
            return SP_FAVORITE;
        if (!n->own && k == SP_HOLD_DOWN) {
            page(n, SP_DELETE);
            return SP_NONE;
        }
        if (k == SP_OK && n->tab == 2)
            page(n, SP_CONTACTS);
        else if (k == SP_UP)
            n->tab = (n->tab + 2) % 3;
        else if (k == SP_DOWN || k == SP_OK)
            n->tab = (n->tab + 1) % 3;
        return SP_NONE;
    }
    if (n->page == SP_EDIT_WIFI || n->page == SP_EDIT_URL) {
        if (k == SP_OK)
            n->page = n->page == SP_EDIT_WIFI ? SP_EDIT_URL : SP_EDIT_WIFI;
        return SP_NONE;
    }
    if (n->page == SP_QR) {
        if (k == SP_OK)
            page(n, SP_CONTACTS);
        return SP_NONE;
    }
    int count = 0;
    switch (n->page) {
    case SP_HOME:
    case SP_SETTINGS:
        count = 4;
        break;
    case SP_INBOX:
        count = n->people;
        break;
    case SP_OWN:
    case SP_DELETE:
        count = 2;
        break;
    case SP_CONTACTS:
        count = 3;
        break;
    default:
        break;
    }
    if (!count)
        return SP_NONE;
    if (k == SP_UP)
        n->selected = (n->selected + count - 1) % count;
    if (k == SP_DOWN)
        n->selected = (n->selected + 1) % count;
    if (k != SP_OK)
        return SP_NONE;
    int choice = n->selected;
    switch (n->page) {
    case SP_HOME:
        if (choice == 0)
            page(n, SP_INBOX);
        if (choice == 1)
            page(n, SP_OWN);
        if (choice == 2) {
            page(n, SP_EDIT_WIFI);
            return SP_EDIT_START;
        }
        if (choice == 3)
            page(n, SP_SETTINGS);
        break;
    case SP_INBOX:
        n->own = false;
        n->tab = 0;
        page(n, SP_CARD);
        return SP_OPEN_PEER;
    case SP_OWN:
        if (choice == 0) {
            page(n, SP_EDIT_WIFI);
            return SP_EDIT_START;
        }
        n->own = true;
        n->tab = 0;
        page(n, SP_CARD);
        return SP_OPEN_OWN;
    case SP_CONTACTS:
        n->contact = choice;
        page(n, SP_QR);
        break;
    case SP_SETTINGS:
        if (choice == 0)
            return SP_PAUSE;
        if (choice == 1)
            return SP_SOUND;
        if (choice == 2)
            return SP_DIM;
        page(n, SP_INBOX);
        break;
    case SP_DELETE:
        page(n, choice ? SP_INBOX : SP_CARD);
        if (choice)
            return SP_REMOVE;
        break;
    default:
        break;
    }
    return SP_NONE;
}
