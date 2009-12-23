#include <libvmebus.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct vme_mapping  map;
    void                *p;

    memset(&map, 0, sizeof(map));
    //map.am          = VME_A24_SUP_DATA_SCT;
    map.am          = VME_A16_USER;
    map.data_width  = VME_D16;
    map.sizel       = 0x100;
    map.vme_addrl   = 0xF70000;

    if((p = vme_map(&map, 0)) == NULL)
    {
        printf("Mapping failed\n");
        return(1);
    }    
    printf("Mapping okay\n");
    return(0);
}

// EOF
