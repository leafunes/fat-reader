#include <stdio.h>
#include <stdlib.h>

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

void print_file_info(Fat12Entry *entry) {
    switch(entry->attr) {
    case 0x00:
        return; // unused entry
    /*case 0xE5:
        printf("Archivo borrado: [?%.7s.%.3s]\n", // COMPLETAR
        return;
    case 0x05:
        printf("Archivo que comienza con 0xE5: [%c%.7s.%.3s]\n", 0xE5, // COMPLETAR 
        break;*/
    case 0x0F:
        return;
    case 0x10:
        printf("Directorio: [%s%s]\n",entry->dos_name, entry->dos_ext); // COMPLETAR 
        break;
    default:
        if(entry->dos_name[0] == 0xE5)
            return;
        printf("Archivo: [%.8s.%.3s]\n", entry->dos_name, entry->dos_ext); // COMPLETAR 
    }
    
}

int main() {
    FILE * in = fopen("test.img", "rb");
    int first_fat12_partition;
    PartitionTable pt[4];
    Fat12BootSector bs;
    Fat12Entry entry;
    
    fseek(in, 0x1BE, SEEK_SET); // Ir al inicio de la tabla de particiones
    fread(pt, sizeof(PartitionTable), 4, in); // leo entradas 
    
    for(first_fat12_partition=0; first_fat12_partition<4; first_fat12_partition++) {        
        printf("Tipo de particion: %d\n", pt[first_fat12_partition].partition_type);
        if(pt[first_fat12_partition].partition_type == 1) {
            printf("Encontrado FAT12 en %d\n", first_fat12_partition);
            break;
        }
    }
    
    if(first_fat12_partition == 4) {
        printf("No se encontró filesystem FAT12, saliendo ...\n");
        return -1;
    }

    fseek(in, 0, SEEK_SET);
    fread(&bs, sizeof(Fat12BootSector), 1, in);
    printf("En  0x%X, sector size %d, FAT size %d sectors, %d FATs\n\n", 
           (unsigned int)ftell(in), bs.bytes_per_sector, bs.sectors_per_fat, bs.fat_quant);
           
    PartitionTable first_partition = pt[first_fat12_partition];
    unsigned int start_addr_FAT12 = first_partition.logic_block_addr * bs.bytes_per_sector;
    unsigned int addr_first_FAT12 = (first_partition.logic_block_addr) * bs.bytes_per_sector;
    unsigned int addr_root_dir = addr_first_FAT12 + (bs.fat_quant * bs.sectors_per_fat * bs.bytes_per_sector);
    
    printf("Primera particion Fat12 address: 0x%X\n", start_addr_FAT12);
    printf("Primera Fat12 address: 0x%X\n", addr_first_FAT12);
    printf("Root Dir address: 0x%X\n", addr_root_dir);

    fseek(in, start_addr_FAT12, SEEK_CUR);

    printf("Root dir_entries %d \n", bs.max_root_entries);

    for(int i=0; i<bs.max_root_entries; i++) {
        fread(&entry, sizeof(entry), 1, in);
        print_file_info(&entry);
    }
    
    printf("\nLeido Root directory, ahora en 0x%X\n", ftell(in));
    fclose(in);
    return 0;
}
