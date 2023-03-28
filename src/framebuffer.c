#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    uint16_t pos = r * RESOLUTION_WIDTH + c;
	out(CURSOR_PORT_CMD, 0x0F);
	out(CURSOR_PORT_DATA, (uint8_t) (pos & 0xFF));
	out(CURSOR_PORT_CMD, 0x0E);
	out(CURSOR_PORT_DATA, (uint8_t) ((pos >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    uint8_t attrib = (bg << 4) | (fg & 0x0F);
    uint8_t *where;
    where = MEMORY_FRAMEBUFFER + (row * RESOLUTION_WIDTH * 2 + col*2);
    memset(where, c, 1);
    memset(where+1, attrib, 1);
}

void framebuffer_clear(void) {
    int size = RESOLUTION_WIDTH * RESOLUTION_HEIGHT;
    uint8_t *where = MEMORY_FRAMEBUFFER;
    for (int i = 0; i < size; i++) {
        memset(where, 0, 1);
        memset(where+1, 0x07, 1);
        where += 2;
    }
}

uint16_t framebuffer_get_cursor(void) {
    uint16_t pos = 0;
    out(CURSOR_PORT_CMD, 0x0F);
    pos |= in(CURSOR_PORT_DATA);
    out(CURSOR_PORT_CMD, 0x0E);
    pos |= ((uint16_t) in(CURSOR_PORT_DATA)) << 8;

   return pos;
}

void framebuffer_scroll(void) {
    uint8_t *where;

    /* copying characters */
    for (int row = 0; row < RESOLUTION_HEIGHT - 1; row++) {
        for (int col = 0; col < RESOLUTION_WIDTH; col++) {
            where = MEMORY_FRAMEBUFFER + ((row + 1) * RESOLUTION_WIDTH * 2 + col*2);
            framebuffer_write(row, col, *where, 0xF, 0x0);
        }
    }

    /* clear last row */
    for (int col = 0; col < RESOLUTION_WIDTH; col++) {
        framebuffer_write(RESOLUTION_HEIGHT - 1, col, 0x0, 0xF, 0x0);
    }
}