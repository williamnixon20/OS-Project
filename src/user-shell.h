#ifndef _SHELL_H
#define _SHELL_H

struct nameStruct{
    char name[8];
    char ext[3];
};

int get_last_idx(unsigned char *buff);
void print_cwd();

void get_input();

void where_is();
void print_output();
void get_cluster_name(char *buff, int clus_num);
void cat();
void mv();
void cp();
void rm();
void clear_buffer_out();
void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);
void populate_ls();
void fill_table_entry();
void addBuf(unsigned char *buff, char *string);
void clear_buffer(unsigned char *buff);
void make_dir();
int getFile(int start_ind, struct nameStruct *file1, struct nameStruct *file2, bool single);
#endif