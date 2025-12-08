#ifndef FAT_H
#define FAT_H

#include <stdint.h>
#include <stddef.h>

/* Boot sector data structure */
struct boot_sector {
    char code[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t num_sectors_per_cluster;
    uint16_t num_reserved_sectors;
    uint8_t num_fat_tables;
    uint16_t num_root_dir_entries;
    uint16_t total_sectors;
    uint8_t media_descriptor;
    uint16_t num_sectors_per_fat;
    uint16_t num_sectors_per_track;
    uint16_t num_heads;
    uint32_t num_hidden_sectors;
    uint32_t total_sectors_in_fs;
    uint8_t logical_drive_num;
    uint8_t reserved;
    uint8_t extended_signature;
    uint32_t serial_number;
    char volume_label[11];
    char fs_type[8];
    char boot_code[448];
    uint16_t boot_signature;
} __attribute__((packed));

/* Root Directory Entry */
struct dir_entry {
    char name[8];
    char ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t high_cluster;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t low_cluster;
    uint32_t file_size;
} __attribute__((packed));

/* File attributes */
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LFN        0x0F

/* FAT file handle */
typedef struct {
    uint16_t cluster;
    uint32_t size;
    uint32_t position;
} fat_file_t;

/* Function prototypes */
int fatInit(void);
int fatOpen(const char *filename, fat_file_t *file);
int fatRead(fat_file_t *file, char *buffer, uint32_t size);

#endif
