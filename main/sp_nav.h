#pragma once
#include <stdbool.h>
typedef enum {
    SP_HOME,
    SP_INBOX,
    SP_OWN,
    SP_CARD,
    SP_CONTACTS,
    SP_QR,
    SP_EDIT_WIFI,
    SP_EDIT_URL,
    SP_SETTINGS,
    SP_DELETE,
    SP_NOTICE,
    SP_ERROR
} sp_page_t;
typedef enum { SP_UP, SP_DOWN, SP_OK, SP_BACK, SP_HOLD_UP, SP_HOLD_DOWN } sp_key_t;
typedef enum {
    SP_NONE,
    SP_OPEN_PEER,
    SP_OPEN_OWN,
    SP_EDIT_START,
    SP_EDIT_STOP,
    SP_FAVORITE,
    SP_REMOVE,
    SP_PAUSE,
    SP_SOUND,
    SP_DIM
} sp_action_t;
typedef struct {
    sp_page_t page;
    int selected, tab, people, contact;
    bool own;
} sp_nav_t;
sp_action_t sp_nav_key(sp_nav_t *n, sp_key_t key);
