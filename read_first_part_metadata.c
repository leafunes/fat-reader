#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE * in = fopen("test.img", "rb");
    unsigned int length_sectors;
    
    fseek(in, 0x1BE, SEEK_SET); // Voy a la tabla de particiones
    
    printf("Booteable: 0x%02X\n", fgetc(in));
    printf("Cylinder: 0x%02X\n", fgetc(in));
    printf("Head: 0x%02X\n", fgetc(in));
    printf("Sector: 0x%02X\n", fgetc(in));
    printf("Partition type 0x%02X\n", fgetc(in));
    
    fseek(in, 7, SEEK_CUR);

    fread(&length_sectors, 4, 1, in);
    printf("Sectors: %d\n",length_sectors);

    fclose(in);
    return 0;
}
