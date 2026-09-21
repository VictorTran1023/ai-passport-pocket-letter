#include "sp_core.h"
#include <string.h>
const uint16_t sp_limits[SP_FIELD_COUNT] = {48, 96, 144, 96, 80, 48, 64};
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t n) {
    for (int i = 0; i < 4; i++)
        p[i] = (uint8_t)(n >> (8 * i));
}
static uint32_t crc(const uint8_t *p, size_t n) {
    uint32_t c = UINT32_MAX;
    while (n--) {
        c ^= *p++;
        for (int i = 0; i < 8; i++)
            c = (c >> 1) ^ ((c & 1) ? 0xedb88320u : 0);
    }
    return ~c;
}
bool sp_utf8(const char *s, size_t n) {
    if (!s)
        return false;
    for (size_t i = 0; i < n;) {
        uint8_t b = (uint8_t)s[i++];
        uint32_t cp;
        int more;
        if (b < 0x80) {
            if (b < 0x20 || b == 0x7f)
                return false;
            continue;
        }
        if (b >= 0xc2 && b <= 0xdf) {
            cp = b & 31;
            more = 1;
        } else if (b >= 0xe0 && b <= 0xef) {
            cp = b & 15;
            more = 2;
        } else if (b >= 0xf0 && b <= 0xf4) {
            cp = b & 7;
            more = 3;
        } else
            return false;
        if (i + (size_t)more > n)
            return false;
        for (int k = 0; k < more; k++) {
            b = (uint8_t)s[i++];
            if ((b & 0xc0) != 0x80)
                return false;
            cp = (cp << 6) | (b & 63);
        }
        if ((more == 2 && cp < 0x800) || (more == 3 && cp < 0x10000) || cp > 0x10ffff ||
            (cp >= 0xd800 && cp <= 0xdfff))
            return false;
        /* Hide neither bidirectional controls nor embedded C1 controls in public cards. */
        if ((cp >= 0x80 && cp <= 0x9f) || (cp >= 0x202a && cp <= 0x202e) ||
            (cp >= 0x2066 && cp <= 0x2069))
            return false;
    }
    return true;
}
bool sp_card_valid(const sp_card_t *c) {
    if (!c || !c->revision)
        return false;
    uint8_t any = 0;
    for (int j = 0; j < 8; j++)
        any |= c->id[j];
    if (!any)
        return false;
    for (int f = 0; f < SP_FIELD_COUNT; f++) {
        size_t n = 0;
        while (n < SP_TEXT_CAP && c->text[f][n])
            n++;
        if (n > sp_limits[f] || !sp_utf8(c->text[f], n) || (f == 0 && !n))
            return false;
    }
    return true;
}
size_t sp_encode(const sp_card_t *c, uint8_t out[SP_WIRE_MAX]) {
    if (!out || !sp_card_valid(c))
        return 0;
    memcpy(out, "SP01", 4);
    memcpy(out + 4, c->id, 8);
    put32(out + 12, c->revision);
    size_t off = 18;
    for (int f = 0; f < SP_FIELD_COUNT; f++) {
        size_t n = strlen(c->text[f]);
        out[off++] = (uint8_t)n;
        memcpy(out + off, c->text[f], n);
        off += n;
    }
    size_t total = off + 4;
    out[16] = (uint8_t)total;
    out[17] = (uint8_t)(total >> 8);
    put32(out + off, crc(out, off));
    return total;
}
bool sp_decode(sp_card_t *c, const uint8_t *w, size_t n) {
    if (!c || !w || n < 29 || n > SP_WIRE_MAX || memcmp(w, "SP01", 4) ||
        ((size_t)w[16] | (size_t)w[17] << 8) != n || get32(w + n - 4) != crc(w, n - 4))
        return false;
    sp_card_t value = {0};
    memcpy(value.id, w + 4, 8);
    value.revision = get32(w + 12);
    size_t off = 18;
    for (int f = 0; f < SP_FIELD_COUNT; f++) {
        if (off >= n - 4)
            return false;
        size_t len = w[off++];
        if (len > sp_limits[f] || off + len > n - 4)
            return false;
        memcpy(value.text[f], w + off, len);
        if (memchr(w + off, 0, len))
            return false;
        off += len;
    }
    if (off != n - 4 || !sp_card_valid(&value))
        return false;
    *c = value;
    return true;
}
int sp_rx_chunk(sp_rx_t *r, uint16_t offset, uint16_t total, const uint8_t *data, size_t len) {
    if (!r || !data || !total || total > SP_WIRE_MAX || !len || len > SP_CHUNK_SIZE ||
        (size_t)offset + len > total)
        return -1;
    if (r->used == 0) {
        if (offset)
            return -1;
        r->total = total;
    }
    if (total != r->total)
        return -1;
    if (offset < r->used)
        return offset + len <= r->used && !memcmp(r->data + offset, data, len) ? (r->used == total)
                                                                               : -1;
    if (offset != r->used)
        return -1;
    memcpy(r->data + offset, data, len);
    r->used += (uint16_t)len;
    return r->used == total;
}
int sp_slot(const sp_index_t *ix, size_t count, const uint8_t id[8]) {
    int empty = -1, oldest = -1;
    for (size_t i = 0; i < count; i++) {
        if (ix[i].occupied && !memcmp(ix[i].id, id, 8))
            return (int)i;
        if (!ix[i].occupied && empty < 0)
            empty = (int)i;
        if (ix[i].occupied && !ix[i].favorite && (oldest < 0 || ix[i].seen < ix[oldest].seen))
            oldest = (int)i;
    }
    return empty >= 0 ? empty : oldest;
}
bool sp_initiates(const uint8_t local[8], const uint8_t remote[8]) {
    return memcmp(local, remote, 8) < 0;
}
