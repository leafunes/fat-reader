#ifndef FAT12_UNGS
#define FAT12_UNGS


typedef struct {
    unsigned char status;
    unsigned char start_chs[3];
    unsigned char partition_type;
    unsigned char last_chs[3];
    unsigned int logic_block_addr;
    unsigned int total_blocks;
} __attribute((packed)) PartitionTable;

typedef struct {
    unsigned char jmp[3];
    char oem[8];
    unsigned short bytes_per_sector;
    unsigned char sector_per_cluster;
    unsigned short reserved_sectors;
    unsigned char fat_quant;
    unsigned short max_root_entries;
    unsigned short total_sectors_short;
    unsigned char media_descriptor;
    unsigned short sectors_per_fat;
    unsigned short sectors_per_track;
    unsigned short numbers_of_heads;
    unsigned int hidden_sectors;    
    unsigned int total_sectors_long;
    unsigned char volume_number;
    unsigned char flags;
    unsigned char signature;
    unsigned int volume_id;
    char volume_label[11];
    char fs_type[8]; // Type en ascii
    char boot_code[448];
    unsigned short boot_sector_signature;
} __attribute((packed)) Fat12BootSector;

typedef struct {
    unsigned char dos_name[8];
    char dos_ext[3];
    unsigned char attr;
    unsigned char reserved;
    unsigned char created_time[5];
    unsigned char last_access_year[2];
    unsigned char ea_index[2];
    unsigned char last_access_time[2];
    unsigned char last_access_date[2];
    unsigned short first_cluster;
    unsigned int filesize;
    
} __attribute((packed)) Fat12Entry;

#endif