#include "fat.h"


FAT_Status_enum FAT_Init(FAT_Descriptor_t* local)
{
    /* Read Master boot record */
    if (SD_SingleRead(local->card, 0, local->buffer) != 0) return FAT_DiskError;
    /* Read LBAs */
    uint8_t counter = 0;
    uint8_t type_code;
    uint8_t* ptr = local->buffer + FAT_MBR_Partition0;
    /* Find not-empty partition */
    while (counter < 4)
    {
        type_code = ptr[FAT_Partition_TypeCode];
        if ((type_code == 0x0B) || (type_code == 0x0C)) break;
        counter += 1;
        ptr += FAT_MBR_Partition_Length;
    }
    if (counter == 4) return FAT_DiskNForm;
    /* Read LBA startaddr. It is a start address of file system */
    local->lba_begin = (uint32_t)(ptr[FAT_Partition_LBA_Begin+3]<<24) |
        (uint32_t)(ptr[FAT_Partition_LBA_Begin+2]<<16) |
        (uint32_t)(ptr[FAT_Partition_LBA_Begin+1]<<8) |
        (uint32_t)ptr[FAT_Partition_LBA_Begin];
    /* Read LBA sector */
    if (SD_SingleRead(local->card, local->lba_begin, local->buffer) != 0) return FAT_DiskError;

    local->sec_per_clust = local->buffer[FAT_BPB_SecPerClus];
    /* Read number of sectors of reserved file system's region */
    uint16_t num_of_res_sec = (uint16_t)(local->buffer[FAT_BPB_RsvdSecCnt+1]<<8) |
        local->buffer[FAT_BPB_RsvdSecCnt];
    /* Read FAT's startaddesses and length */
    local->fat1_begin = local->lba_begin + num_of_res_sec;
    local->num_of_fats = local->buffer[FAT_BPB_NumFATs];
    local->fat_length = (uint32_t)(local->buffer[FAT_BPB_FATSz32+3]<<24) |
        (uint32_t)(local->buffer[FAT_BPB_FATSz32+2]<<16) |
        (uint32_t)(local->buffer[FAT_BPB_FATSz32+1]<<8) |
        (uint32_t)(local->buffer[FAT_BPB_FATSz32]);
    if (local->num_of_fats == 2) local->fat2_begin = local->fat1_begin + local->fat_length;
    /* Calculate a start address of file system's data region */
    local->cluster_begin = local->fat1_begin + local->num_of_fats * local->fat_length;
    return FAT_OK;
}



FAT_Status_enum FAT_CreateDir(FAT_Descriptor_t* local, char* name)
{
    /* Найти номер конечного кластера root-директории */
    uint32_t* ptr = local->buffer;
    uint32_t idx = 0;
    ptr[0] = 0;
    do {
        idx = ptr[idx%512];
        if (SD_SingleRead(local->card, local->fat1_begin+(idx/512), local->buffer) != 0) return FAT_DiskError;
    } while ((ptr[idx%512] & 0x0FFFFFFF) < 0x0FFFFFF7);
    //(idx*sec_per_clust+fat1_begin) - number of the last rootdir's cluster

    uint8_t i=0;
    while (i<local->sec_per_clust)
    {
        if (SD_SingleRead(local->card, idx+local->fat1_begin+i, local->buffer) != 0) return FAT_DiskError;
        uint16_t j=0;
        while (j<512)
        {
            if (local->buffer[j] == 0x00) break;
            j += 32;
        }
        i += 1;
    }


    if (SD_SingleRead(local->card, local->fat1_begin, local->buffer) != 0) return FAT_DiskError;
    //if ((ptr[0] & 0x0FFFFFFF) < 0x0FFFFFF7)
}



FAT_Status_enum FAT_FindByName(FAT_Descriptor_t* local, char* name)
{
    char name_str[12];

    uint8_t pos = 0;
    bool ready = false;
    while ((name[pos] != '\0') && !ready)
    {
        /* Обнаружена точка */
        if (name[pos] == '.')
        {
            if (pos > 8) return FAT_Error;
            memcpy(name_str, name, pos);
            for (uint8_t i=pos; i<8; i++) name_str[i] = 0x20;
            uint8_t i=0;
            while (name[pos+1+i] != '\0')
            {
                if (pos+i > 11) return FAT_Error;
                name_str[8+i] = name[pos+1+i];
                i += 1;
            }
            for (uint8_t j=i; j<11-8; j++) name_str[j] = 0x20;
            ready = true;
        }
        pos += 1;
    }
    /* Точка не обнаружена */
    if (!ready)
    {
        uint8_t i=0;
        while (name[i] != '\0')
        {
            if (i > 8) return FAT_Error;
            name_str[i] = name[i];
            i += 1;
        }
        for ((void)i; i<11; i++) name_str[i] = 0x20;
    }

    if (SD_SingleRead(local->card, local->cluster_begin+local->fs_pointer, local->buffer) != 0) return FAT_DiskError;

    uint16_t i=0;
    while ((i < 512) && memcmp(name_str, local->buffer+i, 11)) i += 32;
    if (i == 512) return FAT_NotFound;

    local->fs_pointer = (uint32_t)local->buffer[i+FAT_DIR_FstClusHI+0]<<16 |
        (uint32_t)local->buffer[i+FAT_DIR_FstClusHI+1]<<24 |
        (uint32_t)local->buffer[i+FAT_DIR_FstClusLO+0] |
        (uint32_t)local->buffer[i+FAT_DIR_FstClusLO+1]<<8;
    local->fs_len = (uint32_t)local->buffer[i+FAT_DIR_FileSize+0] |
        (uint32_t)local->buffer[i+FAT_DIR_FileSize+1]<<8 |
        (uint32_t)local->buffer[i+FAT_DIR_FileSize+2]<<16 |
        (uint32_t)local->buffer[i+FAT_DIR_FileSize+3]<<24;
    return FAT_OK;
}