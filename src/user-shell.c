#include "lib-header/stdmem.h"
#include "lib-header/stdtype.h"
#include "filesystem/fat32.h"
#include "user-shell.h"

struct ClusterBuffer cwd;
struct ClusterBuffer inBuf;
struct ClusterBuffer outBuf;

struct FAT32DirectoryTable fat_table;
int cluster_hist[50] = {ROOT_CLUSTER_NUMBER};
int cluster_whereis[50] = {};
struct ClusterBuffer temp;

int get_last_cluster_index()
{
    int index = 0;
    while (cluster_hist[index] != 0)
    {
        index += 1;
    }
    return index;
}

int get_top()
{
    return cluster_hist[get_last_cluster_index() - 1];
}

int get_top_prev()
{
    return cluster_hist[get_last_cluster_index() - 2];
}

void add_cluster_hist(int new_cluster)
{
    int last_index = get_last_cluster_index();
    cluster_hist[last_index] = new_cluster;
}

void pop_cluster_hist()
{
    int last_index = get_last_cluster_index();
    if (last_index != 1)
    {
        cluster_hist[last_index - 1] = 0;
    }
}

struct FAT32DirectoryEntry *dirtable_shell_linear_search(struct FAT32DirectoryEntry *dir_table, char *name, char *ext, bool _isFile)
{
    for (uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {
        bool sameFileName = memcmp(dir_table[i].name, name, 8) == 0;
        bool sameExt = memcmp(dir_table[i].ext, ext, 3) == 0;
        bool isFile = dir_table[i].attribute != ATTR_SUBDIRECTORY;
        if (sameFileName && sameExt && isFile == _isFile)
        {
            return &dir_table[i];
        }
    }
    return 0;
}

void change_dir()
{
    fill_table_entry();
    // ls ____ start from 3, maju sampai endl
    int start_ind = 3;
    struct ClusterBuffer dir_cd;
    clear_buffer(dir_cd.buf);
    while (inBuf.buf[start_ind] != 0)
    {
        dir_cd.buf[start_ind - 3] = inBuf.buf[start_ind];
        start_ind += 1;
    }
    if (memcmp(dir_cd.buf, "..", 2) == 0)
    {
        addBuf(outBuf.buf, "OK, went back.");
        pop_cluster_hist();
        fill_table_entry();
        return;
    }
    struct FAT32DirectoryEntry *dirent = dirtable_shell_linear_search(fat_table.table, (char *)dir_cd.buf, (char *)"\0\0\0", FALSE);
    if (dirent == 0)
    {
        addBuf(outBuf.buf, "Not found or not a dir");
    }
    else
    {
        addBuf(outBuf.buf, "OK, found");
        int parent_cluster_number = (((uint32_t)dirent->cluster_high) << 16) | (dirent->cluster_low);
        add_cluster_hist(parent_cluster_number);
        struct FAT32DriverRequest ls_request = {
            .buf = fat_table.table,
            .name = "",
            .ext = "\0\0\0",
            .parent_cluster_number = get_top_prev(),
            .buffer_size = sizeof(fat_table)};

        memcpy(ls_request.name, dir_cd.buf, 8);
        // Ambil entry table dari parent.
        int retcode;
        syscall(1, (uint32_t)&ls_request, (uint32_t)&retcode, 0);
        parent_cluster_number += retcode;
    }
}

void cat()
{
    struct ClusterBuffer test;
    struct ClusterBuffer dir_cd;
    clear_buffer(dir_cd.buf);
    int start_ind = 4;
    while (inBuf.buf[start_ind] != 0)
    {
        dir_cd.buf[start_ind - 4] = inBuf.buf[start_ind];
        start_ind += 1;
    }
    clear_buffer(outBuf.buf);
    struct FAT32DriverRequest request = {
        .buf = &test,
        .name = "",
        .ext = "\0\0\0",
        .parent_cluster_number = get_top_prev(),
        .buffer_size = 0x100000,
    };
    memcpy(request.name, dir_cd.buf, 8);
    addBuf(outBuf.buf, request.buf);
    struct FAT32DirectoryEntry *dirent = dirtable_shell_linear_search(fat_table.table, (char *)dir_cd.buf, (char *)"\0\0\0", TRUE);

    if (dirent)
    {
        addBuf(outBuf.buf, "The file is found!\n");
        int32_t retcode;
        syscall(0, (uint32_t)&request, (uint32_t)&retcode, 0);
        addBuf(outBuf.buf, request.buf);
    }
    else
    {
        addBuf(outBuf.buf, "The file doen't exist!");
    }
    // still cant show the buffer?
}
void cp()
{
    // format: cp source newfile
    // todo: read the source and copy to the new buffer
    // done: create new file
    struct ClusterBuffer dir_cd;
    struct ClusterBuffer source;
    struct ClusterBuffer new;
    int start_id = 3;

    while (inBuf.buf[start_id] != 0)
    {
        dir_cd.buf[start_id - 3] = inBuf.buf[start_id];
        start_id += 1;
    }
    clear_buffer(outBuf.buf);

    // todo : read file kok gamau bzz
    struct FAT32DriverRequest request1 = {
        .buf = (uint8_t *)0,
        .name = "",
        .ext = "\0\0\0",
        .parent_cluster_number = get_top(),
        .buffer_size = 0x100000,
    };

    // write file
    struct FAT32DriverRequest request2 = {
        .buf = (uint8_t *)0,
        .name = "",
        .ext = "\0\0\0",
        .parent_cluster_number = get_top(),
        .buffer_size = 0x1000,
    };
    memcpy(request1.name, dir_cd.buf, 16);

    start_id = 3;
    // get the source
    while (inBuf.buf[start_id] != 0)
    {
        if (request1.name[start_id - 3] != ' ')
        {
            source.buf[start_id - 3] = inBuf.buf[start_id];
            start_id += 1;
        }
        else
        {
            break;
        }
    }
    start_id += 1;
    // get the copy name
    int i = 0;
    while (inBuf.buf[start_id] != 0)
    {
        new.buf[i] = inBuf.buf[start_id];
        start_id += 1;
        i++;
    }
    memcpy(request2.name, new.buf, 8);
    memcpy(request1.name, source.buf, 8);

    struct FAT32DirectoryEntry *dirent = dirtable_shell_linear_search(fat_table.table, (char *)source.buf, (char *)"\0\0\0", TRUE);
    if (dirent)
    {
        int retcode;
        syscall(2, (uint32_t)&request2, (uint32_t)&retcode, 0);
    }
    else
    {
        addBuf(outBuf.buf, "File is not found! \n");
    }
    clear_buffer(dir_cd.buf);
    clear_buffer(new.buf);
    clear_buffer(source.buf);
}
void rm()
{
    // msh blm bs keremove :)
    struct ClusterBuffer dir_cd;
    int start_id = 3;
    while (inBuf.buf[start_id] != 0)
    {
        dir_cd.buf[start_id - 3] = inBuf.buf[start_id];
        start_id += 1;
    }
    clear_buffer(outBuf.buf);
    struct FAT32DriverRequest request = {
        .buf = &dir_cd,
        .name = "",
        .ext = "\0\0\0",
        .parent_cluster_number = get_top(),
        .buffer_size = 0x100000,
    };
    memcpy(request.name, dir_cd.buf, 8);
    int32_t retcode;
    syscall(8, (uint32_t)&request, (uint32_t)&retcode, 0);
}

void mv()
{
}

int main(void)
{
    memcpy(cwd.buf, "root", 4);
    fill_table_entry();
    while (TRUE)
    {
        print_cwd();
        get_input();
        if (memcmp(inBuf.buf, "ls", 2) == 0)
        {
            populate_ls();
        }
        else if (memcmp(inBuf.buf, "cd", 2) == 0)
        {
            change_dir();
        }
        else if (memcmp(inBuf.buf, "mkdir", 5) == 0)
        {
            make_dir();
        }
        else if (memcmp(inBuf.buf, "whereis", 7) == 0)
        {
            where_is();
        }
        else if (memcmp(inBuf.buf, "cat", 3) == 0)
        {
            cat();
        }
        else if (memcmp(inBuf.buf, "rm", 2) == 0)
        {
            rm();
        }
        else if (memcmp(inBuf.buf, "mv", 2) == 0)
        {
            mv();
        }
        else if (memcmp(inBuf.buf, "cp", 2) == 0)
        {
            cp();
        }
        else
        {
            clear_buffer(outBuf.buf);
            addBuf(outBuf.buf, "wakarimasen...");
        }
        print_output();
    }

    return 0;
}
void clear_whereis()
{
    for (int i = 0; i < 50; i++)
    {
        cluster_whereis[i] = 0;
    }
}
void reconstruct_path()
{
    int index = 0;
    clear_buffer(outBuf.buf);
    bool found = FALSE;
    while (cluster_whereis[index] != 0)
    {
        found = TRUE;
        clear_buffer(temp.buf);
        get_cluster_name((char *)temp.buf, cluster_whereis[index]);
        addBuf(outBuf.buf, "/");
        addBuf(outBuf.buf, (char *)temp.buf);
        index += 1;
    }
    if (!found)
    {
        addBuf(outBuf.buf, "Not found");
    }
}
void where_is()
{
    int start_ind = 8;
    struct ClusterBuffer dir;
    clear_buffer(dir.buf);
    clear_buffer(outBuf.buf);
    while (inBuf.buf[start_ind] != 0)
    {
        dir.buf[start_ind - 8] = inBuf.buf[start_ind];
        start_ind += 1;
    }

    clear_whereis();
    struct FAT32DriverRequest request = {
        .buf = cluster_whereis,
        .name = "",
        .ext = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = CLUSTER_SIZE,
    };
    memcpy(request.name, dir.buf, 8);

    int retcode;
    syscall(9, (uint32_t)&request, (uint32_t)&retcode, 0);
    if (retcode == -1)
    {
        clear_whereis();
    }
    reconstruct_path();
}

void make_dir()
{
    int start_ind = 6;
    struct ClusterBuffer dir;
    clear_buffer(dir.buf);
    while (inBuf.buf[start_ind] != 0)
    {
        dir.buf[start_ind - 6] = inBuf.buf[start_ind];
        start_ind += 1;
    }

    struct FAT32DriverRequest request = {
        .buf = (uint8_t *)0,
        .name = "",
        .ext = "\0\0\0",
        .parent_cluster_number = get_top(),
        .buffer_size = 0,
    };
    memcpy(request.name, dir.buf, 8);
    int retcode;
    syscall(2, (uint32_t)&request, (uint32_t)&retcode, 0);
}

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx)
{
    __asm__ volatile("mov %0, %%ebx"
                     : /* <Empty> */
                     : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx"
                     : /* <Empty> */
                     : "r"(ecx));
    __asm__ volatile("mov %0, %%edx"
                     : /* <Empty> */
                     : "r"(edx));
    __asm__ volatile("mov %0, %%eax"
                     : /* <Empty> */
                     : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

// void insert_string(char* buff) {

// }

void fill_table_entry()
{
    int top = get_top();
    struct FAT32DriverRequest ls_request = {
        .buf = fat_table.table,
        .name = "root",
        .ext = "\0\0\0",
        .parent_cluster_number = top,
        .buffer_size = sizeof(fat_table)};
    // kalo bukan di root, coba yang lain.
    if (top != ROOT_CLUSTER_NUMBER)
    {
        ls_request.parent_cluster_number = get_top_prev();
        clear_buffer(temp.buf);
        get_cluster_name((char *)temp.buf, get_top());
        memcpy(ls_request.name, temp.buf, 8);
        memcpy(ls_request.ext, "\0\0\0", 3);
    }

    // Ambil entry table dari parent.
    int retcode;
    syscall(1, (uint32_t)&ls_request, (uint32_t)&retcode, 0);
}
void populate_ls()
{
    fill_table_entry();

    struct ClusterBuffer baru;
    clear_buffer(baru.buf);
    // int curr_idx = 0;
    // Traverse setiap file entry, cek apakah not empty. kalo not empty masukin buffer buat di pritn.
    for (unsigned int i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {
        if (fat_table.table[i].user_attribute == UATTR_NOT_EMPTY)
        {
            if (fat_table.table[i].attribute == ATTR_SUBDIRECTORY)
            {
                addBuf(baru.buf, "DIR: ");
                addBuf(baru.buf, fat_table.table[i].name);
                // memcpy(baru.buf+curr_idx, "DIR: ", 5);
            }
            else
            {
                addBuf(baru.buf, "FILE: ");
                addBuf(baru.buf, fat_table.table[i].name);
                addBuf(baru.buf, ".");
                addBuf(baru.buf, fat_table.table[i].ext);
                // memcpy(baru.buf+curr_idx, "FIL: ", 5);
            }
            addBuf(baru.buf, "\n");
        }
    }
    // Copy ke output buffer.
    memcpy(outBuf.buf, baru.buf, CLUSTER_SIZE);
    // syscall(5, (uint32_t) baru.buf, 255, 0xA);
};

int get_last_idx(unsigned char *buff)
{
    int start = 0;
    while (buff[start] != 0)
    {
        start += 1;
    }
    return start;
}

void get_cluster_name(char *buff, int clus_num)
{
    struct FAT32DriverRequest cwd_request = {
        .buf = buff,
        .name = "",
        .ext = "",
        .parent_cluster_number = clus_num,
        .buffer_size = CLUSTER_SIZE};
    int ret_code;
    syscall(10, (uint32_t)&cwd_request, (uint32_t)&ret_code, 0);
}

void print_cwd()
{
    clear_buffer(cwd.buf);
    struct ClusterBuffer temp;
    clear_buffer(temp.buf);
    addBuf(cwd.buf, "root");

    for (int i = 1; i < get_last_cluster_index(); i++)
    {
        if (i == 1)
        {
            addBuf(cwd.buf, "/");
        }
        get_cluster_name((char *)temp.buf, cluster_hist[i]);
        addBuf(cwd.buf, (char *)temp.buf);
        if (i != get_last_cluster_index() - 1)
        {
            addBuf(cwd.buf, "/");
        }
        clear_buffer(temp.buf);
    }

    int last_index = get_last_idx(cwd.buf);
    cwd.buf[last_index] = '>';
    cwd.buf[last_index + 1] = '\0';
    syscall(5, (uint32_t)cwd.buf, get_last_idx(cwd.buf), 0xA);
    cwd.buf[last_index] = '\0';
}

void get_input()
{
    syscall(4, (uint32_t)inBuf.buf, 255, 0);
}

void print_output()
{
    // memcpy(outBuf.buf, inBuf.buf, CLUSTER_SIZE);
    syscall(5, (uint32_t)outBuf.buf, get_last_idx(outBuf.buf), 0xB);
    syscall(5, (uint32_t) "\n", 2, 0xB);
    clear_buffer(outBuf.buf);
}

void clear_buffer(unsigned char *buff)
{
    for (int i = 0; i < CLUSTER_SIZE; i++)
    {
        buff[i] = 0;
    }
}

void addBuf(unsigned char *buff, char *string)
{
    int index = 0;
    while (buff[index] != 0)
    {
        index += 1;
    }
    int str_ind = 0;
    while (string[str_ind] != 0)
    {
        buff[index] = string[str_ind];
        str_ind += 1;
        index += 1;
    }
    buff[index] = 0;
    return;
}