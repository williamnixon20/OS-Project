#include "lib-header/stdmem.h"
#include "lib-header/stdtype.h"
#include "filesystem/fat32.h"
#include "user-shell.h"

struct ClusterBuffer cwd;
struct ClusterBuffer inBuf;
struct ClusterBuffer outBuf;

int main(void) {
    memcpy(cwd.buf, "root", 4);
    while (TRUE) {
        print_cwd();
        get_input();
        print_output();
    }

    return 0;
}

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}


int get_last_idx(char* buff) {
    int start = 0;
    while (buff[start] != 0) {
        start += 1;
    }
    return start;
}

void print_cwd() {
    int last_index = get_last_idx((char*)cwd.buf);
    cwd.buf[last_index] = '>';
    cwd.buf[last_index+1] = '\0';
    syscall(5, (uint32_t) cwd.buf, 255, 0xA);
    cwd.buf[last_index] = '\0';
}   

void get_input() {
    syscall(4, (uint32_t) inBuf.buf, 255, 0);
}

void print_output() {
    memcpy(outBuf.buf, inBuf.buf, CLUSTER_SIZE);
    syscall(5, (uint32_t) outBuf.buf, 255, 0xB);
    syscall(5, (uint32_t) "\n", 255, 0xB);
}
