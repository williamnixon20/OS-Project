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
  initialize_filesystem_fat32();

    struct ClusterBuffer cbuf[5];
    for (uint32_t i = 0; i < 5; i++)
        for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
            cbuf[i].buf[j] = i + 'a';

    struct FAT32DriverRequest request = {
        .buf                   = cbuf,
        .name                  = "ikanaide",
        .ext                   = "uwu",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0,
    } ;

    write(request);  // Create folder "ikanaide"
    memcpy(request.name, "kano1\0\0\0", 8);
    write(request);  // Create folder "kano1"
    memcpy(request.name, "ikanaide", 8);
    memcpy(request.ext, "\0\0\0", 3);
    _delete(request); // Delete first folder, thus creating hole in FS

    memcpy(request.name, "daijoubu", 8);
    request.buffer_size = 5*CLUSTER_SIZE;
    write(request);  // Create fragmented file "daijoubu"

    struct ClusterBuffer readcbuf;
    read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+1, 1); 
    // If read properly, readcbuf should filled with 'a'

    request.buffer_size = CLUSTER_SIZE;
    int resbabibu = read(request);   // Failed read due not enough buffer size
    struct ClusterBuffer cbuf2[5];
    for (uint32_t i = 0; i < 5; i++)
        for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
            cbuf[i].buf[j] = 0;
    request.buf = cbuf2;
    request.buffer_size = 5*CLUSTER_SIZE;
    int resbabibu2 = read(request);   // Success read on file "daijoubu"
    resbabibu = resbabibu2 + resbabibu;

  activate_keyboard_interrupt();
  while (TRUE)
  {
    keyboard_state_activate();
  }
}
