#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat12.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

unsigned int get_addr_of_cluster(unsigned int addr_data, unsigned short cluster, Fat12BootSector* boot_sector){
    return addr_data + ((cluster - 2) * boot_sector->sector_per_cluster * boot_sector->bytes_per_sector);
}

unsigned int get_addr_of_file(unsigned int addr_data, Fat12Entry* entry, Fat12BootSector* boot_sector){
    return get_addr_of_cluster(addr_data, entry->first_cluster, boot_sector);
}

//Lee un archivo, y lo mete en el buffer
void read_file(unsigned char* buffer, unsigned int addr_data, Fat12Entry* entry, Fat12BootSector* boot_sector, unsigned short* FAT, FILE* file){

    unsigned short cluster = FAT[entry->first_cluster];
    unsigned int total_bytes = entry->filesize; 
    unsigned int bytes_to_read = MIN(total_bytes, boot_sector->sector_per_cluster * boot_sector->bytes_per_sector);
    unsigned int bytes_readed = 0;
    unsigned int file_addr = get_addr_of_cluster(addr_data, entry->first_cluster, boot_sector);

    fseek(file, file_addr, SEEK_SET);
    fread(buffer, sizeof(unsigned char), bytes_to_read, file);

    while(cluster < 0xFF8 && cluster != 0){

        total_bytes -= bytes_to_read;
        bytes_readed += bytes_to_read;
        bytes_to_read = MIN(total_bytes, boot_sector->sector_per_cluster * boot_sector->bytes_per_sector);
        file_addr = get_addr_of_cluster(addr_data, cluster, boot_sector);
        cluster = FAT[cluster];

        fseek(file, file_addr, SEEK_SET);
        fread(buffer + bytes_readed, sizeof(unsigned char), bytes_to_read, file);

    }

}

void print_all_files(unsigned int addr, unsigned int addr_data, unsigned int max_entries,int level, Fat12BootSector* boot_sector, FILE* file){

    Fat12Entry entries[max_entries];
    fseek(file, addr, SEEK_SET);
    fread(entries, sizeof(Fat12Entry), max_entries, file);

    // Tabulacion
    int level_str_len = (level * 2) + 4;
    char level_str[level_str_len];
    memset(level_str, (int)' ', level_str_len);
    memset(level_str, (int)'-', level_str_len - 2);
    level_str[0] = '|';
    level_str[level_str_len - 1] = 0;

    for(int i=0; i < max_entries; i++) {
        //Si es un entry especial, lo ignoro
        if(entries[i].attr == 0x0 || entries[i].attr == 0x0F)
            continue;

        printf("%s", level_str);

        //Si es un directorio, leo las cosas de adentro
        if(entries[i].attr == 0x10){
            printf("[%s%s]\n",entries[i].dos_name, entries[i].dos_ext);
            //Parece que FAT usa directorios especiales, como "."" y ".."" -> esos no hay que leerlos
            if(entries[i].first_cluster != 0){
                unsigned int next_add = get_addr_of_file(addr_data, &entries[i], boot_sector);
                unsigned int next_max_entries = boot_sector->bytes_per_sector / 32;
                if(next_add != addr)
                    print_all_files(next_add, addr_data, next_max_entries, level + 1, boot_sector, file);
            }
        }
        //Si es un file comun, imprimo el nombre
        else if(entries[i].attr == 0x20){
            if(entries[i].dos_name[0] != 0xE5)
                printf("%.8s.%.3s\n", entries[i].dos_name, entries[i].dos_ext);
            else
                printf("*DELETED* %.8s.%.3s\n", entries[i].dos_name, entries[i].dos_ext);
            
        }
    }

    //free(&level_str);
}

//Busca parte de un archivo, lo restaura, y devuelve la ruta
char* find_and_restore(unsigned int addr, unsigned int addr_data, unsigned int max_entries, unsigned short* FAT, int fat_size, Fat12BootSector* boot_sector, FILE* file, char* partial_content){

    Fat12Entry entries[max_entries];
    fseek(file, addr, SEEK_SET);
    fread(entries, sizeof(Fat12Entry), max_entries, file);

    for(int i=0; i < max_entries; i++) {
        //Si es un entry especial, lo ignoro
        if(entries[i].attr == 0x0 || entries[i].attr == 0x0F)
            continue;

        //Si es un directorio, leo las cosas de adentro
        if(entries[i].attr == 0x10){
            //Parece que FAT usa directorios especiales, como "."" y ".."" -> esos no hay que leerlos
            if(entries[i].first_cluster != 0){
                unsigned int next_add = get_addr_of_file(addr_data, &entries[i], boot_sector);
                unsigned int next_max_entries = boot_sector->bytes_per_sector / 32;
                if(next_add != addr)
                    find_and_restore(next_add, addr_data, next_max_entries, FAT, fat_size, boot_sector, file, partial_content);
            }
        }
        //Si es un file comun, lo cargo en memoria, y busco el 
        else if(entries[i].attr == 0x20){
            if(entries[i].dos_name[0] != 0xE5 && entries[i].filesize != 0){

                //Se inicializa en cero
                unsigned int size = entries[i].filesize;
                unsigned char* buffer = calloc(size, sizeof(unsigned char));

                read_file(buffer, addr_data, &entries[i], boot_sector, FAT, file);

                printf("Content of %.8s.%.3s\n", entries[i].dos_name, entries[i].dos_ext);
                printf("%s", buffer);

                //free(buffer);
            }
            
        }
    }

}

int main(){
    FILE * iso = fopen("test.img", "rb");
    int first_fat12_partition;
    PartitionTable partition_table[4]; 
    Fat12BootSector boot_sector;

    // Se leen las entradas de la tabla de particiones
    fseek(iso, 0x1BE, SEEK_SET);
    fread(partition_table, sizeof(PartitionTable), 4, iso);
    
    //Se busca la primera particion fat12
    for(int i = 0; i < 4; i++) {        
        printf("Tipo de particion: %d\n", partition_table[i].partition_type);
        if(partition_table[i].partition_type == 1) {
            printf("Encontrado FAT12 en %d\n", i);
            first_fat12_partition = i;
            break;
        }

        //Si no se encuentra particion, se sale con error
        if(i == 4){
            printf("No se encontró filesystem FAT12, saliendo ...\n");
            return -1;
        }
    }
    
    //Se lee el MBR, y se imprime un poco de info
    fseek(iso, 0, SEEK_SET);
    fread(&boot_sector, sizeof(Fat12BootSector), 1, iso);

    printf("--------MBR--------\n");
    printf("OEM: [%.8s]\n", boot_sector.oem);
    printf("Type: [%.8s]\n", boot_sector.fs_type);
    printf("Bytes per sectors: %u\n", boot_sector.bytes_per_sector);
    printf("Sectors per cluster: %u\n", boot_sector.sector_per_cluster);

    //Se lee la primera particion y se hacen calculos de direccionamiento
    PartitionTable first_partition = partition_table[first_fat12_partition];
    unsigned int addr_FAT12_partition = first_partition.logic_block_addr * boot_sector.bytes_per_sector;
    unsigned int addr_FAT12_table = (first_partition.logic_block_addr) * boot_sector.bytes_per_sector;
    unsigned int addr_root_dir = addr_FAT12_table + (boot_sector.fat_quant * boot_sector.sectors_per_fat * boot_sector.bytes_per_sector);
    unsigned int addr_data_reg = addr_root_dir + (boot_sector.max_root_entries * 32);

    printf("-----PARTITION-----\n");
    printf("Booteable: 0x%01X\n", first_partition.status);
    printf("CHS: 0x%03X\n", first_partition.start_chs);
    printf("Type: 0x%01X\n", first_partition.partition_type);
    printf("Size in sectors: %u\n", first_partition.total_blocks);

    printf("-------FAT12-------\n");
    printf("Primera particion Fat12 address: 0x%X\n", addr_FAT12_partition);
    printf("Primera Fat12 address: 0x%X\n", addr_FAT12_table);
    printf("Root Dir address: 0x%X\n", addr_root_dir);

    //Leo la tabla fat
    //Cuenta rara. cada entrada tiene 12 bits -> 1.5 bytes
    int fat12_size = boot_sector.sectors_per_fat * boot_sector.bytes_per_sector ;
    int fat12_entries = ((fat12_size * 2) / 3);
    unsigned short FAT[fat12_entries]; 
    fseek(iso, addr_FAT12_table, SEEK_SET);

    for(int i = 0; i < fat12_entries; i = i + 2){
        unsigned char low;
        unsigned char mid;
        unsigned char high;

        fread(&low, 1, 1, iso);
        fread(&mid, 1, 1, iso);
        fread(&high, 1, 1, iso);

        unsigned short first = (low << 4) + (mid & 0x0F);
        unsigned short second = (high << 4) + ((mid & 0xF0) >> 4);

        FAT[i] = first;
        FAT[i + 1] = second;
    }

    //Muestro la tabla FAT
    printf("-------FAT12 Entries-------\n");

    for(int i = 0; i < fat12_entries; i++){
        if(i != 0 && i % 16 == 0)
            printf("\n");

        printf("0x%03X ", FAT[i]);
    }
    printf("\n");
    printf("\n");

    //Empiezo a leer los archivos
    printf("-------List Files-------\n");
    print_all_files(addr_root_dir, addr_data_reg, boot_sector.max_root_entries, 0, &boot_sector, iso);
    find_and_restore(addr_root_dir, addr_data_reg, boot_sector.max_root_entries, &FAT, fat12_entries, &boot_sector, iso, "");
    
    return 0;

}