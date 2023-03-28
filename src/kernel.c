#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "interrupt/idt.h"
#include "interrupt/interrupt.h"
#include "keyboard/keyboard.h"

void kernel_setup(void)
{
  // uint32_t a;
  // uint32_t volatile b = 0x0000BABE;
  // __asm__("mov $0xCAFE0000, %0" : "=r"(a));
  // while (TRUE) b += 1;
  enter_protected_mode(&_gdt_gdtr);
  pic_remap();
  initialize_idt();
  framebuffer_clear();
  framebuffer_set_cursor(0, 0);
  __asm__("int $0x4");
  while (TRUE)
  {
    keyboard_state_activate();
  }
}
