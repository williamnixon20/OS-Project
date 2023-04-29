#include "lib-header/stdmem.h"
#include "lib-header/stdtype.h"
#include "filesystem/fat32.h"
#include "user-shell.h"

struct ClusterBuffer cwd;
struct ClusterBuffer inBuf;
struct ClusterBuffer outBuf;
struct ClusterBuffer outBufTest[4];

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

struct FAT32DirectoryEntry *dirtable_shell_linear_search(struct FAT32DirectoryEntry *dir_table, char *name, char *ext, bool isFile)
{
    for (uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {
        bool sameFileName = memcmp(dir_table[i].name, name, 8) == 0;
        bool sameExt = memcmp(dir_table[i].ext, ext, 3) == 0;
        bool sameType = (dir_table[i].attribute != ATTR_SUBDIRECTORY) == isFile;
        // Wild card, whatever extension
        if (memcmp(ext, "\0\0\0", 3) == 0) sameExt = TRUE;
        if (sameFileName && sameExt && sameType)
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

    struct nameStruct arg;
    int ret = getFile(3, &arg, 0);

    if (ret == 1) {
        addBuf(outBuf.buf, "Invalid Input format\n");
        return;
    }

    struct FAT32DirectoryEntry *dirent = dirtable_shell_linear_search(fat_table.table, (char *)arg.name, "\0\0\0", FALSE);
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

void cat(){
    clear_buffer(outBuf.buf);

    struct nameStruct arg;
    int ret = getFile(4, &arg, 0);

    if (ret == 1) {
        addBuf(outBuf.buf, "Invalid input");
        return;
    }

    // struct ClusterBuffer test[4];
    // clear_buffer(test.buf);
    struct FAT32DriverRequest request = {
        .buf = outBufTest,
        .name = "",
        .ext = "\0\0\0",
        .parent_cluster_number = get_top(),
        .buffer_size = 4*CLUSTER_SIZE,
    };
    memcpy(request.name, arg.name, 8);
    memcpy(request.ext, arg.ext, 3);

    int32_t retcode;
    syscall(0, (uint32_t)&request, (uint32_t)&retcode, 0);
    // addBuf(outBufTest.buf, test.buf);
    if (retcode != 0) {
        addBuf(outBuf.buf, "Something went wrong! Probably file not found or size exceeds 4 cluster.");
    } else {
        // addBuf(outBuf.buf, request.buf);
    }
}

void clearStruct(struct nameStruct *file) {
    for (int i = 0; i < 8; i++) {
        file->name[i] = 0;
    }
    for (int i = 0; i < 3; i++) {
        file->ext[i] = 0;
    }
}


int getFile(int start_ind, struct nameStruct *file1, struct nameStruct *file2){
    int start_id = start_ind;
    bool extension = FALSE;
    int ext_ind = 0;
    int name_ind = 0;
    char detect = 0;
    bool found = FALSE;
    clearStruct(file1);
    if (file2) {
        detect = ' ';   
        clearStruct(file2); 
    }

    // Fill first file
    while (inBuf.buf[start_id] != 0 && found == 0)
    {
        // if (inBuf.buf[start_id] == '/') {
        //     start_id += 1;
        //     continue;
        // }
        if (inBuf.buf[start_id] != detect)
        {
            if (inBuf.buf[start_id] == '.') {
                extension = TRUE;
            } else {
                if (extension) {
                    if (ext_ind >= 3) return 1;
                        file1->ext[ext_ind] = inBuf.buf[start_id];
                    ext_ind += 1;
                } else {
                    if (name_ind >= 8) return 1;
                    file1->name[name_ind] = inBuf.buf[start_id];
                    name_ind += 1;
                }
            }
        }
        else
        {
            found = 1;
        }
        start_id += 1;
    }

    if (file2) {
        extension = FALSE;
        ext_ind = 0;
        name_ind = 0;
        found = FALSE;
        while (inBuf.buf[start_id] != 0 && found == 0)
        {
            // if (inBuf.buf[start_id] == '/') {
            //     start_id += 1;
            //     continue;
            // }
            if (inBuf.buf[start_id] != 0)
            {
                if (inBuf.buf[start_id] == '.') {
                    extension = TRUE;
                } else {
                if (extension) {
                    if (ext_ind >= 3) return 1;
                    file2->ext[ext_ind] = inBuf.buf[start_id];
                    ext_ind += 1;
                } else {
                    if (name_ind >= 8) return 1;
                    file2->name[name_ind] = inBuf.buf[start_id];
                    name_ind += 1;
                }
                }
            }
            else
            {
                found = 1;
            }
            start_id += 1;
        }
    } 
    if (start_id==start_ind || name_ind == 0) {
        return 1;
    }
    return 0;
}

void cp()
{
    // // format: cp source newfile
    // // todo: read the source and copy to the new buffer
    // // done: create new file
    // struct ClusterBuffer source;
    // struct ClusterBuffer new;
    // struct ClusterBuffer sourceBuf;
    
    // struct nameStruct arg1;
    // struct nameStruct arg2;
    // int ret = getFile(3, &arg1, &arg2);

    // // if (ret == 1) {
    // //     addBuf(outBuf.buf, "Input invalid");
    // //     return;
    // // }

    // // clear_buffer(outBuf.buf);
    // // int start_id = 0;
    // // bool found = 0;
    // // while (inBuf.buf[start_id+3] != 0 && found == 0)
    // // {
    // //     if (inBuf.buf[start_id+3] != ' ')
    // //     {
    // //         source.buf[start_id] = inBuf.buf[start_id+3];
    // //     }
    // //     else
    // //     {
    // //         found = 1;
    // //     }
    // //     start_id += 1;
    // // }

    // // // get the copy name
    // // int i = 0;
    // // while (inBuf.buf[start_id+3] != 0)
    // // {
    // //     new.buf[i] = inBuf.buf[start_id+3];
    // //     start_id += 1;
    // //     i++;
    // // }
    
    // // if (i == 0) {
    // //     return;
    // // }

    // // CHECK FOR ".."

    // struct FAT32DirectoryEntry *dirent = dirtable_shell_linear_search(fat_table.table, arg1.name, arg1.txt, TRUE);
    // if (!dirent)
    // {
    //     addBuf(outBuf.buf, "File is not found! \n");
    //     return;
    // }
    // struct FAT32DirectoryEntry *direntFile = dirtable_shell_linear_search(fat_table.table, arg2.name, arg2.ext, TRUE);
    // struct FAT32DirectoryEntry *direntFolder = dirtable_shell_linear_search(fat_table.table, arg2.name, arg2.ext, FALSE);
    // // No file or folder exist, just create a new file.
    // if (!direntFile && !direntFolder) {
        
    // // There is file, overwrite
    // } else if (direntFile) {
    // // There is folder, move
    // } else if (direntFolder) {

    // }
    // // Delete copy
    // struct FAT32DriverRequest request_del = {
    //     .buf = 0,
    //     .name = "",
    //     .ext = "\0\0\0",
    //     .parent_cluster_number = get_top(),
    //     .buffer_size = 0,
    // };
    // memcpy(request_del.name, new.buf, 8);
    // int32_t retcode;
    // syscall(8, (uint32_t)&request_del, (uint32_t)&retcode, 0);


    // // todo : read file kok gamau bzz
    // struct FAT32DriverRequest request1 = {
    //     .buf = &sourceBuf,
    //     .name = "",
    //     .ext = "\0\0\0",
    //     .parent_cluster_number = get_top(),
    //     .buffer_size = CLUSTER_SIZE,
    // };
    // clear_buffer(request1.buf);
    // memcpy(request1.name, source.buf, 8);
    // syscall(0, (uint32_t)&request1, (uint32_t)&retcode, 0);

    // // write file
    // struct FAT32DriverRequest request2 = {
    //     .buf = &sourceBuf,
    //     .name = "",
    //     .ext = "\0\0\0",
    //     .parent_cluster_number = get_top(),
    //     .buffer_size = CLUSTER_SIZE,
    // };
    // memcpy(request2.name, new.buf, 8);
    // syscall(2, (uint32_t)&request2, (uint32_t)&retcode, 0);
}

void rm()
{
    clear_buffer(outBuf.buf);
    struct nameStruct arg;
    int ret = getFile(3, &arg, 0);

    if (ret == 1) {
        addBuf(outBuf.buf, "Invalid input");
        return;
    }

    struct FAT32DriverRequest request = {
        .buf = 0,
        .name = "",
        .ext = "\0\0\0",
        .parent_cluster_number = get_top(),
        .buffer_size = 0,
    };
    memcpy(request.name, arg.name, 8);
    memcpy(request.ext, arg.ext, 3);
    int32_t retcode;
    syscall(8, (uint32_t)&request, (uint32_t)&retcode, 0);
    if (retcode != 0) {
        addBuf(outBuf.buf, "Something went wrong! File probably not found");
    }
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
        else if (memcmp(inBuf.buf, "cd ", 3) == 0)
        {
            change_dir();
        }
        else if (memcmp(inBuf.buf, "mkdir ", 6) == 0)
        {
            make_dir();
        }
        else if (memcmp(inBuf.buf, "whereis ", 8) == 0)
        {
            where_is();
        }
        else if (memcmp(inBuf.buf, "cat ", 4) == 0)
        {
            cat();
        }
        else if (memcmp(inBuf.buf, "rm ", 3) == 0)
        {
            rm();
        }
        else if (memcmp(inBuf.buf, "mv ", 3) == 0)
        {
            mv();
        }
        else if (memcmp(inBuf.buf, "cp ", 3) == 0)
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
        clear_buffer(temp.buf);  syscall(5, (uint32_t)outBuf.buf, CLUSTER_SIZE, 0xB);
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
    clear_buffer(outBuf.buf);
    
    // int start_ind = 8;
    // struct ClusterBuffer dir;
    // clear_buffer(dir.buf);
    // while (inBuf.buf[start_ind] != 0)
    // {
    //     dir.buf[start_ind - 8] = inBuf.buf[start_ind];
    //     start_ind += 1;
    // }
    struct nameStruct arg;
    int ret = getFile(8, &arg, 0);
    
    if (ret == 1) {
        addBuf(outBuf.buf, "Invalid input");
        return;
    }


    clear_whereis();
    struct FAT32DriverRequest request = {
        .buf = cluster_whereis,
        .name = "",
        .ext = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = CLUSTER_SIZE,
    };
    memcpy(request.name, arg.name, 8);
    memcpy(request.ext, arg.ext, 3);

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
        if (inBuf.buf[start_ind] == '.') {
            addBuf(outBuf.buf, "Directory cannot contain . or extension");
            return;
        }
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
    if (dir.buf[8] != 0) {
        addBuf(outBuf.buf, "Name too long");
        return;
    }
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
    struct ClusterBuffer buff;
    // int curr_idx = 0;
    // Traverse setiap file entry, cek apakah not empty. kalo not empty masukin buffer buat di pritn.
    for (unsigned int i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {
        if (fat_table.table[i].user_attribute == UATTR_NOT_EMPTY)
        {
            if(fat_table.table[i].attribute != ATTR_SUBDIRECTORY) addBuf(baru.buf, "FILE: ");
            else addBuf(baru.buf, "FOLDER: ");
            clear_buffer(buff.buf);
            memcpy(buff.buf, fat_table.table[i].name, 8);
            addBuf(baru.buf, (char *) buff.buf);
            clear_buffer(buff.buf);
            if (fat_table.table[i].attribute != ATTR_SUBDIRECTORY) addBuf(baru.buf, " EXT: ");
            addBuf(baru.buf, fat_table.table[i].ext);
            // addBuf(baru.buf, " SIZE: ");
            // addBuf(baru.buf, (char*) ((fat_table.table[i].filesize/CLUSTER_SIZE) + '0'));
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
    syscall(5, (uint32_t)outBuf.buf, CLUSTER_SIZE-1, 0xB);
    syscall(5, (uint32_t) "\n", 2, 0xB);
    clear_buffer(outBuf.buf);
    if (outBufTest[0].buf[0] != 0) {
        syscall(5, (uint32_t)outBufTest, (CLUSTER_SIZE*4), 0xB);
        syscall(5, (uint32_t) "\n", 2, 0xB);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < CLUSTER_SIZE; j++) {
                outBufTest[i].buf[j] = '\0';
            }
        }
    }
    clear_buffer_out();
}

void clear_buffer(unsigned char *buff)
{
    for (int i = 0; i < CLUSTER_SIZE; i++)
    {
        buff[i] = 0;
    }
}

void clear_buffer_out()
{
    for (int i = 0; i < 4 * CLUSTER_SIZE; i++)
    {
        outBufTest[0].buf[i] = 0;
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