#include "sp_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static sp_card_t card(void) {
    sp_card_t c = {.id = {1, 2, 3, 4, 5, 6, 7, 8}, .revision = 1};
    strcpy(c.text[0], "Test");
    strcpy(c.text[1], "Coffee");
    return c;
}
int main(void) {
    sp_card_t a = card(), b;
    uint8_t wire[SP_WIRE_MAX];
    assert(sp_utf8("\xe4\xb8\xad", 3));
    assert(!sp_utf8("\xc0\xaf", 2));
    assert(!sp_utf8("\xed\xa0\x80", 3));
    assert(!sp_utf8("\xf4\x90\x80\x80", 4));
    assert(!sp_utf8("\xe4\xb8", 2));
    assert(!sp_utf8("\x1b", 1));
    size_t n = sp_encode(&a, wire);
    assert(n > 20 && n <= SP_WIRE_MAX);
    assert(sp_decode(&b, wire, n));
    assert(!memcmp(&a, &b, sizeof a));
    assert(!sp_decode(&b, wire, n - 1));
    wire[n - 1] ^= 1;
    assert(!sp_decode(&b, wire, n));
    wire[n - 1] ^= 1;
    memset(a.text[0], 'a', 49);
    a.text[0][49] = 0;
    assert(!sp_card_valid(&a));
    a = card();
    a.revision = 0;
    assert(!sp_card_valid(&a));
    sp_rx_t rx = {0};
    assert(sp_rx_chunk(&rx, 1, n, wire, 14) == -1);
    assert(sp_rx_chunk(&rx, 0, n, wire, 14) == 0);
    assert(sp_rx_chunk(&rx, 0, n, wire, 14) == 0);
    assert(rx.used == 14);
    assert(sp_rx_chunk(&rx, 15, n, wire + 15, 14) == -1);
    for (size_t off = 14; off < n;) {
        size_t len = n - off > 14 ? 14 : n - off;
        int done = sp_rx_chunk(&rx, off, n, wire + off, len);
        off += len;
        assert(done == (off == n));
    }
    assert(!memcmp(rx.data, wire, n));
    assert(sp_rx_chunk(&rx, 0, SP_WIRE_MAX + 1, wire, 1) == -1);
    sp_index_t ix[3] = {0};
    uint8_t id[8] = {9};
    assert(sp_slot(ix, 3, id) == 0);
    for (int i = 0; i < 3; i++) {
        ix[i].occupied = true;
        ix[i].id[0] = i + 1;
        ix[i].seen = i + 1;
    }
    assert(sp_slot(ix, 3, id) == 0);
    ix[0].favorite = true;
    assert(sp_slot(ix, 3, id) == 1);
    ix[1].favorite = ix[2].favorite = true;
    assert(sp_slot(ix, 3, id) == -1);
    assert(sp_slot(ix, 3, ix[2].id) == 2);
    assert(sp_initiates(ix[0].id, ix[1].id));
    assert(!sp_initiates(ix[1].id, ix[0].id));
    assert(!sp_initiates(id, id));
    a = card();
    for (int f = 0; f < SP_FIELD_COUNT; f++) {
        memset(a.text[f], 'x', sp_limits[f]);
        a.text[f][sp_limits[f]] = 0;
    }
    n = sp_encode(&a, wire);
    assert(n == 605);
    assert(sp_decode(&b, wire, n));
    for (size_t cut = 0; cut < n; cut++)
        assert(!sp_decode(&b, wire, cut));
    for (size_t byte = 0; byte < n; byte++) {
        wire[byte] ^= 0x80;
        assert(!sp_decode(&b, wire, n));
        wire[byte] ^= 0x80;
    }
    rx = (sp_rx_t){0};
    for (size_t off = 0; off < n;) {
        size_t len = n - off > SP_CHUNK_SIZE ? SP_CHUNK_SIZE : n - off;
        int result = sp_rx_chunk(&rx, off, n, wire + off, len);
        off += len;
        assert(result == (off == n));
    }
    uint8_t changed[SP_CHUNK_SIZE];
    memcpy(changed, wire, sizeof changed);
    changed[0] ^= 1;
    assert(sp_rx_chunk(&rx, 0, n, changed, sizeof changed) == -1);
    assert(sp_utf8("\xf0\x9f\x98\x80", 4));
    assert(!sp_utf8("\xf0\x80\x80\xaf", 4));
    puts("StreetPass core: PASS");
    return 0;
}
