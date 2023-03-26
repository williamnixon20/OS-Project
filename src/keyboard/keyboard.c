#include "keyboard.h"
#include "../lib-header/portio.h"
#include "../lib-header/framebuffer.h"
#include "../lib-header/stdmem.h"

static struct KeyboardDriverState keyboard_state = {
  .read_extended_mode = FALSE,
  .keyboard_input_on = TRUE,
  .buffer_index = 0
};

const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

void keyboard_state_activate(void) {
  keyboard_state.keyboard_input_on = TRUE;
}

void keyboard_state_deactivate(void) {
  keyboard_state.keyboard_input_on = FALSE;
}

void get_keyboard_buffer(char *buf) {
  uint8_t buf_idx;
  for (buf_idx = 0; buf_idx <= keyboard_state.buffer_index; buf_idx++) {
      buf[buf_idx] = keyboard_state.keyboard_buffer[buf_idx];
  }
}

bool is_keyboard_blocking(void) {
  return keyboard_state.keyboard_input_on;
}

void keyboard_isr(void) {
    if (!keyboard_state.keyboard_input_on)
        keyboard_state.buffer_index = 0;
    else {
        uint8_t  scancode    = in(KEYBOARD_DATA_PORT);
        char     mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
        // TODO : Implement scancode processing
        if (mapped_char > 0) {
          char     char_buf[KEYBOARD_BUFFER_SIZE];
          keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
          get_keyboard_buffer(char_buf);

          uint8_t row;
          uint8_t col;

          if (mapped_char == '\b' && keyboard_state.buffer_index > 0) {
            keyboard_state.buffer_index--;
            row = keyboard_state.buffer_index / 80;
            col = keyboard_state.buffer_index % 80;
            framebuffer_write(row, col, 0x0, 0xF, 0);
            framebuffer_set_cursor(row, col);
          } else if(mapped_char != '\b') {
            row = keyboard_state.buffer_index / 80;
            col = keyboard_state.buffer_index % 80;

            framebuffer_write(row, col, mapped_char, 0xF, 0);
            keyboard_state.buffer_index++;
            framebuffer_set_cursor(row, col + 1);
          }
          // TODO: backspace
        }
    }
    pic_ack(IRQ_KEYBOARD);
}
