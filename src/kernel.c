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
#include "filesystem/cmostime.h"
#include "paging/paging.h"

// void kernel_setup(void)
// {
// //   // // See time for debug
// //   // read_rtc();
// //   // int s = second;
// //   // int mm = minute;
// //   // int hh = hour;
// //   // int dd = day;
// //   // int mo = month;
// //   // int yr = year;
// //   // s =  s + mm + hh + dd + mo + yr;
// //   enter_protected_mode(&_gdt_gdtr);
// //   pic_remap();
// //   initialize_idt();
// //   activate_keyboard_interrupt();
// //   framebuffer_clear();
// //   initialize_filesystem_fat32();
// //   activate_keyboard_interrupt();

// //   // To avoid warnings error, we use _delete instaed of delete
// //     struct ClusterBuffer cbuf[5];
// //     for (uint32_t i = 0; i < 5; i++)
// //         for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
// //             cbuf[i].buf[j] = i + 'a';

// //     struct FAT32DriverRequest request = {
// //         .buf                   = cbuf,
// //         .name                  = "ikanaide",
// //         .ext                   = "uwu",
// //         .parent_cluster_number = ROOT_CLUSTER_NUMBER,
// //         .buffer_size           = 0,
// //     } ;

// //     write(request);  // Create folder "ikanaide"
// //     memcpy(request.name, "kano1\0\0\0", 8);
// //     write(request);  // Create folder "kano1"
// //     memcpy(request.name, "ikanaide", 8);
// //     memcpy(request.ext, "\0\0\0", 3);
// //     _delete(request); // Delete first folder, thus creating hole in FS

// //     memcpy(request.name, "daijoubu", 8);
// //     memcpy(request.ext, "uwu", 3);
// //     request.buffer_size = 5*CLUSTER_SIZE;
// //     write(request);  // Create fragmented file "daijoubu"

// //     struct ClusterBuffer readcbuf;
// //     read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+1, 1);
// //     // If read properly, readcbuf should filled with 'a'

// //     request.buffer_size = CLUSTER_SIZE;
// //     read(request);   // Failed read due not enough buffer size
// //     struct ClusterBuffer bufferEmpty[5];
// //     request.buf = bufferEmpty;
// //     request.buffer_size = 5*CLUSTER_SIZE;
// //     read(request);   // Success read on file "daijoubu"

// //   while (TRUE)
// //   {
// //     keyboard_state_activate();
// //   }
// }

void kernel_setup(void)
{
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    gdt_install_tss();
    set_tss_register();

    // Allocate first 4 MiB virtual memory
    allocate_single_user_page_frame((uint8_t *)0);

    // Write shell into memory
    struct FAT32DriverRequest request = {
        .buf = (uint8_t *)0,
        .name = "shell",
        .ext = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = 0x100000,
    };
    read(request);

    struct ClusterBuffer cbuf[5];
    for (uint32_t i = 0; i < 5; i++)
        for (uint32_t j = 0; j < CLUSTER_SIZE; j++) {
            cbuf[i].buf[j] = 'a' + i;
            if (j % 50 == 0) {
                cbuf[i].buf[j] = '\n';
            }
        }

    struct FAT32DriverRequest request2 = {
        .buf = cbuf,
        .name = "ikanaide",
        .ext = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = 0,
    };
    write(request2); // Create folder "ikanaide"
    // struct FAT32DriverRequest request3 = {
    //     .buf = cbuf,
    //     .name = "ikanaid2",
    //     .ext = "uwu",
    //     .parent_cluster_number = 5,
    //     .buffer_size = 0,
    // };
    // write(request3);
    // create folder ikanaide2

    struct FAT32DriverRequest request4 = {
        .buf = cbuf,
        .name = "hontouni",
        .ext = "abc",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = 4*CLUSTER_SIZE,
    };
    write(request4);
    struct FAT32DriverRequest request5 = {
        .buf = cbuf,
        .name = "hntpndk\0",
        .ext = "abc",
        .parent_cluster_number = 9,
        .buffer_size = 1*CLUSTER_SIZE,
    };
    write(request5);
    // struct FAT32DriverRequest request5 = {
    //     .buf = cbuf,
    //     .name = "hontouni",
    //     .ext = "abc",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size = CLUSTER_SIZE,
    // };
    // write(request4);

    // create file hontouni

    // Set TSS $esp pointer and jump into shell
    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t *)0);

    while (TRUE)
        ;
}
