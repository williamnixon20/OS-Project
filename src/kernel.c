#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "interrupt/idt.h"
#include "interrupt/interrupt.h"
#include "keyboard/keyboard.h"
#include "filesystem/fat32.h"
#include "filesystem/disk.h"

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
  framebuffer_write(0, 0, 0x0, 0xF, 0xF);
  framebuffer_set_cursor(1,3);
  // char* baru = "hahahihi";
  // write_blocks(baru, 0, 1);
  // write_blocks(baru, 1, 1);
  // write_blocks(baru, 2, 1);
  // write_clusters(baru, 0,2);
  //   write_clusters(baru, 2,1);
      // write_clusters(baru, 2,1);
      //   write_clusters(baru, 3,1);
      //     write_clusters(baru, 4,1);
  initialize_filesystem_fat32();
  // char* aneh = "hahaohoho";
  // write_clusters(aneh, 1, 1);
  activate_keyboard_interrupt();
  __asm__("int $0x4");
  while (TRUE)
  {
    keyboard_state_activate();
  }
}
