#include "fat.h"

#define SECTOR_SIZE 512

/* External utility functions - defined in kernel_main */
extern int strlen(const char *s);
extern int strncmp(const char *s1, const char *s2, int n);
extern int strcasecmp(const char *s1, const char *s2);
extern void memcpy(void *dest, void *src, int n);
extern void memset(void *s, int c, int n);
extern int tolower(int c);
extern void print_decimal(uint32_t num);

/* Global FAT state */
static struct boot_sector *bs = NULL;
static char boot_sector_buf[512];
static char fat_table[16 * SECTOR_SIZE];  /* 8KB for FAT table */
static uint32_t root_sector;
static uint32_t data_sector;

/* Simple SD card read stub - you'll need to implement or link the real sd_readblock */
extern int sd_readblock(uint32_t sector, char *buffer, uint32_t count);

/* Print helper - use kernel's print_string */
extern void print_string(char *s);
extern void print_char(char c);
extern void print_hex32(uint32_t val);

int fatInit(void) {
    print_string("FAT: Initializing FAT filesystem...\n");
    
    /* Read boot sector */
    if (sd_readblock(0, boot_sector_buf, 1) != 0) {
        print_string("FAT: Error reading boot sector\n");
        return -1;
    }
    
    bs = (struct boot_sector *)boot_sector_buf;
    
    /* Validate boot signature */
    if (bs->boot_signature != 0xaa55) {
        print_string("FAT: Invalid boot signature: 0x");
        print_hex32(bs->boot_signature);
        print_char('\n');
        return -1;
    }
    
    /* Validate filesystem type */
    if (strncmp(bs->fs_type, "FAT", 3) != 0) {
        print_string("FAT: Invalid filesystem type: ");
        for (int i = 0; i < 8; i++) {
            if (bs->fs_type[i] != ' ') print_char(bs->fs_type[i]);
        }
        print_char('\n');
        return -1;
    }
    
    /* Calculate sector positions */
    root_sector = bs->num_reserved_sectors + 
                  (bs->num_fat_tables * bs->num_sectors_per_fat);
    
    uint32_t root_dir_sectors = ((bs->num_root_dir_entries * 32) + 
                                 (bs->bytes_per_sector - 1)) / bs->bytes_per_sector;
    
    data_sector = root_sector + root_dir_sectors;
    
    /* Read FAT table */
    if (sd_readblock(bs->num_reserved_sectors, fat_table, bs->num_sectors_per_fat) != 0) {
        print_string("FAT: Error reading FAT table\n");
        return -1;
    }
    
    print_string("FAT: Initialized successfully\n");
    print_string("FAT: Bytes per sector: ");
    print_decimal(bs->bytes_per_sector);
    print_char('\n');
    print_string("FAT: Sectors per cluster: ");
    print_decimal(bs->num_sectors_per_cluster);
    print_char('\n');
    
    return 0;
}

int fatOpen(const char *filename, fat_file_t *file) {
    if (!bs) {
        print_string("FAT: Filesystem not initialized\n");
        return -1;
    }
    
    print_string("FAT: Opening file: ");
    print_string((char *)filename);
    print_char('\n');
    
    char dir_sector_buf[512];
    struct dir_entry *entries = (struct dir_entry *)dir_sector_buf;
    
    /* Search root directory */
    uint32_t num_root_entries = bs->num_root_dir_entries;
    uint32_t entries_per_sector = SECTOR_SIZE / sizeof(struct dir_entry);
    
    for (uint32_t sector_idx = 0; sector_idx < (num_root_entries + entries_per_sector - 1) / entries_per_sector; sector_idx++) {
        if (sd_readblock(root_sector + sector_idx, dir_sector_buf, 1) != 0) {
            print_string("FAT: Error reading root directory\n");
            return -1;
        }
        
        for (uint32_t i = 0; i < entries_per_sector && (sector_idx * entries_per_sector + i) < num_root_entries; i++) {
            struct dir_entry *entry = &entries[i];
            
            /* Skip empty entries */
            if (entry->name[0] == 0) {
                continue;
            }
            
            /* Skip deleted entries */
            if ((unsigned char)entry->name[0] == 0xE5) {
                continue;
            }
            
            /* Skip LFN entries */
            if ((entry->attributes & ATTR_LFN) == ATTR_LFN) {
                continue;
            }
            
            /* Build 8.3 name and compare */
            char entry_name[13];
            int pos = 0;
            
            /* Copy name part */
            for (int j = 0; j < 8 && entry->name[j] != ' '; j++) {
                entry_name[pos++] = tolower(entry->name[j]);
            }
            
            /* Copy extension if present */
            int has_ext = 0;
            for (int j = 0; j < 3 && entry->ext[j] != ' '; j++) {
                has_ext = 1;
                break;
            }
            
            if (has_ext) {
                entry_name[pos++] = '.';
                for (int j = 0; j < 3 && entry->ext[j] != ' '; j++) {
                    entry_name[pos++] = tolower(entry->ext[j]);
                }
            }
            
            entry_name[pos] = '\0';
            
            /* Compare filenames */
            if (strcasecmp(entry_name, filename) == 0 && !(entry->attributes & ATTR_DIRECTORY)) {
                print_string("FAT: Found file at cluster ");
                print_decimal(entry->low_cluster);
                print_string(", size ");
                print_decimal(entry->file_size);
                print_char('\n');
                
                file->cluster = entry->low_cluster;
                file->size = entry->file_size;
                file->position = 0;
                return 0;
            }
        }
    }
    
    print_string("FAT: File not found\n");
    return -1;
}

int fatRead(fat_file_t *file, char *buffer, uint32_t size) {
    if (!bs || !file) {
        print_string("FAT: Invalid file handle or filesystem\n");
        return -1;
    }
    
    print_string("FAT: Reading ");
    print_decimal(size);
    print_string(" bytes from file\n");
    
    uint32_t bytes_read = 0;
    uint32_t bytes_to_read = size;
    if (bytes_to_read > file->size - file->position) {
        bytes_to_read = file->size - file->position;
    }
    
    char cluster_buf[4096];  /* Support up to 8 sectors per cluster */
    uint32_t current_cluster = file->cluster;
    uint32_t offset = file->position;
    
    while (bytes_read < bytes_to_read && current_cluster < 0xFFF8) {
        /* Calculate sector for this cluster */
        uint32_t sector = data_sector + ((current_cluster - 2) * bs->num_sectors_per_cluster);
        
        /* Read cluster */
        uint32_t cluster_size = bs->num_sectors_per_cluster * SECTOR_SIZE;
        if (sd_readblock(sector, cluster_buf, bs->num_sectors_per_cluster) != 0) {
            print_string("FAT: Error reading cluster\n");
            return -1;
        }
        
        /* Copy data from cluster to buffer */
        uint32_t to_copy = (bytes_to_read - bytes_read < cluster_size) ? 
                           (bytes_to_read - bytes_read) : cluster_size;
        
        if (offset > 0) {
            /* Skip offset bytes in first cluster */
            to_copy = (cluster_size - offset < to_copy) ? 
                      (cluster_size - offset) : to_copy;
            memcpy(&buffer[bytes_read], &cluster_buf[offset], to_copy);
            offset = 0;
        } else {
            memcpy(&buffer[bytes_read], cluster_buf, to_copy);
        }
        
        bytes_read += to_copy;
        
        /* Get next cluster from FAT */
        uint16_t *fat_entry = (uint16_t *)fat_table;
        current_cluster = fat_entry[current_cluster];
    }
    
    file->position += bytes_read;
    
    print_string("FAT: Read ");
    print_decimal(bytes_read);
    print_string(" bytes\n");
    
    return bytes_read;
}
