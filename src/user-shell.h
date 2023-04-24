#ifndef _SHELL_H
#define _SHELL_H

int get_last_idx(unsigned char* buff);
void print_cwd();

void get_input();

void print_output();
void get_cluster_name(char* buff, int clus_num);

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);
void populate_ls();
void fill_table_entry();
void addBuf(unsigned char* buff, char* string);
void clear_buffer(unsigned char* buff);
#endif