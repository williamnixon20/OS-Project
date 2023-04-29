#include "keyboard.h"
#include "../lib-header/portio.h"
#include "../lib-header/framebuffer.h"
#include "../lib-header/stdmem.h"

static struct KeyboardDriverState keyboard_state = {
    .read_extended_mode = FALSE,
    .keyboard_input_on = FALSE,
    .buffer_index = 0};

const char keyboard_scancode_1_to_ascii_map[256] = {
    0,
    0x1B,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    '\b',
    '\t',
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    '\n',
    0,
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    0,
    '\\',
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    0,
    '*',
    0,
    ' ',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    '-',
    0,
    0,
    0,
    '+',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

void keyboard_state_activate(void)
{
  keyboard_state.keyboard_input_on = TRUE;
}

void keyboard_state_deactivate(void)
{
  keyboard_state.keyboard_input_on = FALSE;
}

void get_keyboard_buffer(char *buf)
{
  uint8_t buf_idx = 0;
  while (keyboard_state.keyboard_buffer[buf_idx] != '\0')
  {
    buf[buf_idx] = keyboard_state.keyboard_buffer[buf_idx];
    buf_idx++;
  }
  buf[buf_idx] = '\0';
}

bool is_keyboard_blocking(void)
{
  return keyboard_state.keyboard_input_on;
}

void keyboard_isr(void)
{
  uint8_t scancode = in(KEYBOARD_DATA_PORT);
  if (!keyboard_state.keyboard_input_on)
  {
    keyboard_state.buffer_index = 0;
  }
  else
  {
    char mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
    if (mapped_char > 0)
    {
      keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;

      uint16_t pos = framebuffer_get_cursor();
      uint8_t row = pos / RESOLUTION_WIDTH;
      uint8_t col = pos % RESOLUTION_WIDTH;

      if (mapped_char == '\b' && keyboard_state.buffer_index > 0)
      {
        row = (pos - 1) / RESOLUTION_WIDTH;
        col = (pos - 1) % RESOLUTION_WIDTH;

        framebuffer_write(row, col, 0x0, 0xF, 0);
        framebuffer_set_cursor(row, col);

        keyboard_state.buffer_index--;
      }
      else if (mapped_char == '\n')
      {
        keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = '\0';
        framebuffer_new_line();
        keyboard_state_deactivate();
        keyboard_state.buffer_index = 0;
      }
      else if (mapped_char != '\b' && keyboard_state.buffer_index < KEYBOARD_BUFFER_SIZE - 1)
      {
        framebuffer_write_buf(&mapped_char, 1, 0xF);
        keyboard_state.buffer_index++;
      }
    }
  }
  pic_ack(IRQ_KEYBOARD);
}
