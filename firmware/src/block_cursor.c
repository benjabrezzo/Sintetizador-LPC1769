#include "block_cursor.h"

void blockCursorInit(BlockCursor *c) {
    c->nextHalf = 0;
}

uint8_t blockCursorNext(BlockCursor *c) {
    const uint8_t half = c->nextHalf;
    c->nextHalf ^= 1;
    return half;
}
