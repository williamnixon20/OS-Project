#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    uint16_t pos = r * 80 + c;
	out(CURSOR_PORT_CMD, 0x0F);
	out(CURSOR_PORT_DATA, (uint8_t) (pos & 0xFF));
	out(CURSOR_PORT_CMD, 0x0E);
	out(CURSOR_PORT_DATA, (uint8_t) ((pos >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    uint8_t attrib = (bg << 4) | (fg & 0x0F);
    uint8_t *where;
    where = MEMORY_FRAMEBUFFER + (row * 160 + col*2);
    memset(where, c, 1);
    memset(where+1, attrib, 1);
}

void framebuffer_clear(void) {
    int size = 80 * 25;
    uint8_t *where = MEMORY_FRAMEBUFFER;
    for (int i = 0; i < size; i++) {
        memset(where, 0, 1);
        memset(where+1, 0x07, 1);
        where += 2;
    }
}
