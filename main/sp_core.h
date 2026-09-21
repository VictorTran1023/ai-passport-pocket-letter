#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define SP_FIELD_COUNT 7
#define SP_TEXT_CAP 145
#define SP_WIRE_MAX 640
#define SP_MAX_PEOPLE 100
#define SP_CHUNK_SIZE 14
/* UTF-8 byte limits, excluding terminators. Wire format is explicitly encoded. */
extern const uint16_t sp_limits[SP_FIELD_COUNT];
typedef struct {
    uint8_t id[8];
    uint32_t revision;
    char text[SP_FIELD_COUNT][SP_TEXT_CAP];
} sp_card_t;
typedef struct {
    uint8_t data[SP_WIRE_MAX];
    uint16_t total, used;
} sp_rx_t;
typedef struct {
    uint8_t id[8];
    uint32_t revision, seen;
    bool occupied, favorite, unread;
    char name[49];
} sp_index_t;
bool sp_utf8(const char *text, size_t length);
bool sp_card_valid(const sp_card_t *card);
size_t sp_encode(const sp_card_t *card, uint8_t out[SP_WIRE_MAX]);
bool sp_decode(sp_card_t *card, const uint8_t *wire, size_t length);
/* 0: incomplete, 1: complete, -1: rejected. Ordered duplicate chunks are harmless. */
int sp_rx_chunk(sp_rx_t *rx, uint16_t offset, uint16_t total, const uint8_t *data, size_t length);
int sp_slot(const sp_index_t *index, size_t count, const uint8_t id[8]);
bool sp_initiates(const uint8_t local[8], const uint8_t remote[8]);
