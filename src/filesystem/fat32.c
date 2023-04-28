#include "../lib-header/stdtype.h"
#include "fat32.h"
#include "disk.h"
#include "../lib-header/stdmem.h"
#include "cmostime.h"

struct FAT32DriverState driver_state = {0};

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

/**
 * Convert cluster number to logical block address
 * 
 * @param cluster Cluster number to convert
 * @return uint32_t Logical Block Address
 */

uint32_t cluster_to_lba(uint32_t cluster) {
    return BOOT_SECTOR + cluster * CLUSTER_BLOCK_COUNT;
};

/**
 * Initialize DirectoryTable value with parent DirectoryEntry and directory name
 * 
 * @param dir_table          Pointer to directory table
 * @param name               8-byte char for directory name
 * @param parent_dir_cluster Parent directory cluster number
 */
void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster){;
    memset(dir_table, 0, sizeof(*dir_table));
    memcpy(dir_table->table[0].name, name, 8);
    dir_table->table[0].cluster_low = (uint16_t) parent_dir_cluster;
    dir_table->table[0].cluster_high = (uint16_t) (parent_dir_cluster >> 16);
    dir_table->table[0].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;
    // dir_table->table[0].create_date = getDateEncode();
    // dir_table->table[0].create_time = getTimeEncode();
    // dir_table->table[0].modified_time = getTimeEncode();
    // dir_table->table[0].modified_date = getDateEncode();
}

/**
 * Checking whether filesystem signature is missing or not in boot sector
 * 
 * @return True if memcmp(boot_sector, fs_signature) returning inequality
 */
bool is_empty_storage(void) {
    struct ClusterBuffer buffer = {0};
    read_blocks(buffer.buf, BOOT_SECTOR, 1);
    if (memcmp(buffer.buf, fs_signature, BLOCK_SIZE) != 0) {
        return TRUE;
    }
    return FALSE;
};

/**
 * Create new FAT32 file system. Will write fs_signature into boot sector and 
 * proper FileAllocationTable (contain CLUSTER_0_VALUE, CLUSTER_1_VALUE, 
 * and initialized root directory) into cluster number 1
 */
void create_fat32(void){
    write_blocks(&fs_signature, BOOT_SECTOR, 1);
    driver_state.fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    driver_state.fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    driver_state.fat_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;
    write_clusters(driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
    struct FAT32DirectoryTable rootTable = {0};
    init_directory_table(&rootTable, "root\0\0\0\0", ROOT_CLUSTER_NUMBER);
    write_clusters(&rootTable, ROOT_CLUSTER_NUMBER, 1);
}

/**
 * Initialize file system driver state, if is_empty_storage() then create_fat32()
 * Else, read and cache entire FileAllocationTable (located at cluster number 1) into driver state
 */
void initialize_filesystem_fat32(void) {
    if (is_empty_storage()) {
        create_fat32();
        refreshFATDriver();
    } else {
        refreshFATDriver();
    }

};

/**
 * Write cluster operation, wrapper for write_blocks().
 * Recommended to use struct ClusterBuffer
 * 
 * @param ptr            Pointer to source data
 * @param cluster_number Cluster number to write
 * @param cluster_count  Cluster count to write, due limitation of write_blocks block_count 255 => max cluster_count = 63
 */
void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

/**
 * Read cluster operation, wrapper for read_blocks().
 * Recommended to use struct ClusterBuffer
 * 
 * @param ptr            Pointer to buffer for reading
 * @param cluster_number Cluster number to read
 * @param cluster_count  Cluster count to read, due limitation of read_blocks block_count 255 => max cluster_count = 63
 */
void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
};




/* -- CRUD Operation -- */

/**
 *  FAT32 Folder / Directory read
 *
 * @param request buf point to struct FAT32DirectoryTable,
 *                name is directory name,
 *                ext is unused,
 *                parent_cluster_number is target directory table to read,
 *                buffer_size must be exactly sizeof(struct FAT32DirectoryTable)
 * @return Error code: 0 success - 1 not a folder - 2 not found - -1 unknown
 */
int8_t read_directory(struct FAT32DriverRequest request) {
    uint32_t parent_cluster = request.parent_cluster_number;
    uint32_t fat_table_content = driver_state.fat_table.cluster_map[parent_cluster]; 

    if (fat_table_content == FAT32_FAT_EMPTY_ENTRY) {
        return 2;
    }

    if (fat_table_content != FAT32_FAT_END_OF_FILE) {
        return 1;
    }

    if (request.buffer_size != CLUSTER_SIZE) {
        return -1;
    }

    read_clusters(request.buf, request.parent_cluster_number, 1);
    struct FAT32DirectoryEntry* entry =  dirtable_linear_search(request.buf, request, FALSE);
    if (!entry) {
        return 2;
    } else {
        // entry->access_date = getDateEncode();
        write_clusters(request.buf, parent_cluster, 1);
        uint32_t folder_cluster_num =  (((uint32_t) entry->cluster_high) << 16)|(entry->cluster_low);
        read_clusters(request.buf, folder_cluster_num, 1);
        return 0;
    }
};

/**
 * FAT32 read, read a file from file system.
 *
 * @param request All attribute will be used for read, buffer_size will limit reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown
 */
int8_t read(struct FAT32DriverRequest request) {
    bool isFile = request.buffer_size != 0;
    
    if (!isFile) {
        return 1;
    }

    struct FAT32DirectoryTable parentFolder = {0};
    if (clusterEmpty(request.parent_cluster_number)) {
        return -2;
    }
    read_clusters(&parentFolder, request.parent_cluster_number, 1);
    if (parentFolder.table[0].attribute != ATTR_SUBDIRECTORY) {
        return -2;
    } 

    struct FAT32DirectoryEntry* entry =  dirtable_linear_search(parentFolder.table, request, isFile);
    if (!entry) {
        return 3;
    }
    if (entry->attribute == ATTR_SUBDIRECTORY) {
        return 1;
    }
    if (entry->filesize > request.buffer_size) {
        return 2;
    }
    // entry->access_date = getDateEncode();
    write_clusters(&parentFolder, request.parent_cluster_number, 1);

    uint32_t cluster_file = (((uint32_t)entry->cluster_high) << 16) | entry->cluster_low;
    int iteration = 0;
    while (TRUE) {
        read_clusters(request.buf + iteration * CLUSTER_SIZE, cluster_file, 1);
        iteration += 1;
        cluster_file = driver_state.fat_table.cluster_map[cluster_file];
        if (cluster_file == FAT32_FAT_END_OF_FILE) {
            return 0;
        }
    }
    return -1;
};
 


/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request) {
    bool isFile = request.buffer_size != 0;
    
    struct FAT32DirectoryTable parentFolder;
    if (clusterEmpty(request.parent_cluster_number)) {
        return 2;
    }
    read_clusters(&parentFolder, request.parent_cluster_number, 1);
    if (parentFolder.table[0].attribute != ATTR_SUBDIRECTORY) {
        return 2;
    } 

    struct FAT32DirectoryEntry* entry =  dirtable_linear_search(parentFolder.table, request, isFile);
    if (entry) {
        return 1;
    }

    if (request.buffer_size == 0) {
        int emptyCluster = getEmptyCluster();
        if (emptyCluster == -1) {
            return -1;
        }
        struct FAT32DirectoryTable newDir = {0};
        memset(&newDir, 0, sizeof(newDir));
        uint32_t parent_cluster = request.parent_cluster_number;
        init_directory_table(&newDir, request.name, parent_cluster);
        write_clusters(&newDir, emptyCluster, 1);

        struct FAT32DirectoryEntry newEntry = {0};
        memset(&newEntry, 0, sizeof(newEntry));
        driver_state.fat_table.cluster_map[emptyCluster] = FAT32_FAT_END_OF_FILE;
        createDirectoryEntry(request, &newEntry, emptyCluster);
        addWriteToParentDir(parentFolder, newEntry, request.parent_cluster_number);
        
    } else {
        int total_cluster = (request.buffer_size + CLUSTER_SIZE-1) / CLUSTER_SIZE;
        int nextEmpty;
        for (int i = 0; i < total_cluster; i++) {
            int emptyCluster = getEmptyCluster();
            driver_state.fat_table.cluster_map[emptyCluster] = -1; // -1 cuma placeholder
            nextEmpty = getEmptyCluster();
            if (emptyCluster == -1 || nextEmpty == -1) {
                return -1;
            }

            write_clusters(request.buf + i*CLUSTER_SIZE, emptyCluster, 1);
            driver_state.fat_table.cluster_map[emptyCluster] = nextEmpty;

            if (i == 0) {
                struct FAT32DirectoryEntry newEntry = {0};
                createDirectoryEntry(request, &newEntry, emptyCluster);
                addWriteToParentDir(parentFolder, newEntry, request.parent_cluster_number);
            }
            if (i == total_cluster-1) {
                driver_state.fat_table.cluster_map[emptyCluster] = FAT32_FAT_END_OF_FILE;
            }
        }
    }
    writeFATDriver();
    refreshFATDriver();
    return 0;
}



/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t _delete(struct FAT32DriverRequest request) {
    struct FAT32DirectoryTable parentFolder = {0};
    read_clusters(&parentFolder, request.parent_cluster_number, 1);
    
    if (parentFolder.table[0].attribute != ATTR_SUBDIRECTORY) {
        return -1;
    } 

    struct FAT32DirectoryEntry* entry =  dirtable_linear_search(parentFolder.table, request, TRUE);
    if (!entry) {
        entry = dirtable_linear_search(parentFolder.table, request, FALSE);
    }
    if (entry) {
        if (entry->attribute == ATTR_SUBDIRECTORY) {
            struct FAT32DirectoryTable currentFolder = {0};
            uint32_t cluster_num =  (((uint32_t) entry->cluster_high) << 16)|(entry->cluster_low);
            read_clusters(&currentFolder, cluster_num, 1);
            
            if (dir_not_empty(currentFolder)) {
                return 2;
            }
            removeFromParentDir(parentFolder, request);
            driver_state.fat_table.cluster_map[cluster_num] = FAT32_FAT_EMPTY_ENTRY;            
        } else {
            uint32_t cluster_num =  (((uint32_t) entry->cluster_high) << 16)|(entry->cluster_low);
            uint32_t currFatIndex = cluster_num;
            removeFromParentDir(parentFolder, request);
            while (currFatIndex != FAT32_FAT_END_OF_FILE) {
                uint32_t holder_val = driver_state.fat_table.cluster_map[currFatIndex];
                driver_state.fat_table.cluster_map[currFatIndex] = FAT32_FAT_EMPTY_ENTRY;
                currFatIndex = holder_val;
            }
            driver_state.fat_table.cluster_map[currFatIndex] = FAT32_FAT_EMPTY_ENTRY;
        }
    } else {
        return -2;
    }
    writeFATDriver();
    refreshFATDriver();
    return 0;
};

void removeFromParentDir(struct FAT32DirectoryTable parentTable, struct FAT32DriverRequest req) {
    bool isFile = req.buffer_size != 0;
    struct FAT32DirectoryEntry emptyEntry = {0};
    memset(&emptyEntry, 0, sizeof(emptyEntry));
    struct FAT32DirectoryEntry *entry = dirtable_linear_search(parentTable.table, req, isFile);
    (*entry) = emptyEntry;
    write_clusters(&parentTable, req.parent_cluster_number, 1);
}

void createDirectoryEntry(struct FAT32DriverRequest request, struct FAT32DirectoryEntry* newEntri, uint32_t cluster_inf) {
    memset(newEntri, 0, sizeof(*newEntri));
    if (request.buffer_size == 0) {
        newEntri->attribute = ATTR_SUBDIRECTORY;
    } else {
        memcpy(newEntri->ext, request.ext, 3);
    }
    memcpy(newEntri->name, request.name, 8);
    // newEntri->create_date = getDateEncode();
    // newEntri->create_time = getTimeEncode();
    // newEntri->modified_time = getTimeEncode();
    // newEntri->modified_date = getDateEncode();
    newEntri->filesize = request.buffer_size;
    newEntri->user_attribute = UATTR_NOT_EMPTY;
    newEntri->cluster_low = (uint16_t) cluster_inf;
    newEntri->cluster_high = (uint16_t) (cluster_inf >> 16);
}

int32_t getEmptyCluster() {
    for (int i = 0; i < CLUSTER_MAP_SIZE; i++) {
        if (driver_state.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY) {;
            return i;
        }
    }
    return -1;
}


bool clusterEmpty(uint32_t cluster_num) {
    uint32_t fat_table_content = driver_state.fat_table.cluster_map[cluster_num]; 

    if (fat_table_content == FAT32_FAT_EMPTY_ENTRY) {
        return TRUE;
    } else {
        return FALSE;
    }
}

bool clusterIsFolder(uint32_t cluster_num) {
    uint32_t fat_table_content = driver_state.fat_table.cluster_map[cluster_num]; 

    if (fat_table_content == FAT32_FAT_END_OF_FILE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void refreshFATDriver() {
    read_clusters(driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
    read_clusters(driver_state.dir_table_buf.table, ROOT_CLUSTER_NUMBER, 1);
}

void writeFATDriver() {
    struct FAT32FileAllocationTable table = driver_state.fat_table;
    write_clusters(&table, FAT_CLUSTER_NUMBER, 1);
}

void addWriteToParentDir(struct FAT32DirectoryTable parentTable, struct FAT32DirectoryEntry entry, uint32_t parentCluster) {
    int capacity = (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry));
    for (int i = 0; i < capacity; i++) {
        if (parentTable.table[i].user_attribute != UATTR_NOT_EMPTY) {
            parentTable.table[i] = entry;
            break;
        }
    }
    write_clusters(&parentTable, parentCluster, 1);
}


bool dir_not_empty(struct FAT32DirectoryTable table) {
    int capacity = (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry));
    for (int i = 1; i < capacity; i++) {
        if (table.table[i].user_attribute == UATTR_NOT_EMPTY) {
            return TRUE;
        }
    }
    return FALSE;
}


struct FAT32DirectoryEntry* dirtable_linear_search(struct FAT32DirectoryEntry *dir_table, struct FAT32DriverRequest request, bool isFile){
    for (uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
        bool sameFileName = memcmp(dir_table[i].name, request.name, 8) == 0;
        bool sameExt = TRUE;
        if (isFile && memcmp(request.ext, "\0\0\0", 3) != 0) {
            sameExt = memcmp(dir_table[i].ext, request.ext, 3) == 0;
        }
        if (sameFileName && sameExt) {
            return &dir_table[i];
        }
    }
    return 0;
}

void get_dir_string(struct FAT32DriverRequest req) {
    struct FAT32DirectoryTable parentFolder = {0};
    read_clusters(&parentFolder, req.parent_cluster_number, 1);
    memcpy(req.buf, parentFolder.table[0].name, 8);
    int* arr = req.buf;
    memcpy(req.buf, arr, 1);
}

int temp_buf[50] = {};
int index = 0;
void clear_temp_buf() {
    index = 0;
    for (int i = 0; i < 50; i++) {
        temp_buf[i] = 0;
    }
}

int populate_path(struct FAT32DriverRequest req) {
    if (req.parent_cluster_number == 2) {
        clear_temp_buf();
    }
    struct FAT32DirectoryTable currFolder = {0};
    read_clusters(currFolder.table, req.parent_cluster_number, 1);
    struct FAT32DirectoryEntry* entry =  dirtable_linear_search(currFolder.table, req, FALSE);
    if (entry) {
        temp_buf[index] = req.parent_cluster_number;
        index += 1;
        memcpy(req.buf, temp_buf, 400);
        return 1;
    } else {
        temp_buf[index] = req.parent_cluster_number;
        index += 1;
        for (uint8_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
            if (currFolder.table[i].attribute == ATTR_SUBDIRECTORY) {
                entry = &currFolder.table[i];
                uint32_t folder_cluster_num =  (((uint32_t) entry->cluster_high) << 16)|(entry->cluster_low);
                req.parent_cluster_number = folder_cluster_num;
                int ret_code = populate_path(req);
                if (ret_code == 1) {
                    return 1;
                }
            }
        }
        temp_buf[index-1] = 0;
        index -= 1;
    }
    return -1;
};