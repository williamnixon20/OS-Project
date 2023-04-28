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

void framebuffer_write_buf(char * buf, uint32_t len, uint8_t color) {
    for (uint32_t idx = 0; idx < len; idx++) {
        if (buf[idx] == 0) break;
        if (buf[idx] == '\n') {
            framebuffer_new_line();
        }
        else {
            uint16_t pos = framebuffer_get_cursor();
            uint8_t row = pos / RESOLUTION_WIDTH;
            uint8_t col = pos % RESOLUTION_WIDTH;
            framebuffer_write(row, col, buf[idx], color, 0x0);

            if (pos + 1 == RESOLUTION_HEIGHT * RESOLUTION_WIDTH) {
                framebuffer_scroll();
                framebuffer_set_cursor(row, 0);
            } else {
                row = (pos + 1) / RESOLUTION_WIDTH;
                col = (pos + 1) % RESOLUTION_WIDTH;
                framebuffer_set_cursor(row, col);
            }
        }
    }
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

void framebuffer_new_line(void) {
    uint16_t pos = framebuffer_get_cursor();
    uint8_t row = pos / RESOLUTION_WIDTH;
    uint8_t col = pos % RESOLUTION_WIDTH;
    if (row == RESOLUTION_HEIGHT - 1) {
        framebuffer_scroll();
        framebuffer_set_cursor(row, 0);
    } else {
        row = (row + 1);
        col = 0;
        framebuffer_set_cursor(row, col);
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
    /* copying characters */
    int size = RESOLUTION_WIDTH * RESOLUTION_HEIGHT;
    uint8_t *where = MEMORY_FRAMEBUFFER;
    for (int i = 0; i < size - RESOLUTION_WIDTH; i++) {
        memcpy(where, where + RESOLUTION_WIDTH * 2, sizeof(where));
        where += 2;
    }

    /* clear last row */
    for (int col = 0; col < RESOLUTION_WIDTH; col++) {
        framebuffer_write(RESOLUTION_HEIGHT - 1, col, 0x0, 0xF, 0x0);
    }
}