#include "fat.h"


FAT_Status_t FAT_Init(FAT_Descriptor_t* fs)
{
    /* Read Master boot record */
    if (SD_SingleRead(fs->card, 0, fs->buffer) != 0) return FAT_DiskError;
    /* Read LBAs */
    uint8_t counter = 0;
    uint8_t type_code;
    uint8_t* ptr = fs->buffer + FAT_MBR_Partition0;
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
    fs->fs_begin = (uint32_t)(ptr[FAT_Partition_LBA_Begin+3]<<24) |
        (uint32_t)(ptr[FAT_Partition_LBA_Begin+2]<<16) |
        (uint32_t)(ptr[FAT_Partition_LBA_Begin+1]<<8) |
        (uint32_t)ptr[FAT_Partition_LBA_Begin];
    /* Read LBA sector */
    if (SD_SingleRead(fs->card, fs->fs_begin, fs->buffer) != 0) return FAT_DiskError;

    fs->param.sec_per_clust = fs->buffer[FAT_BPB_SecPerClus];
    /* Read number of sectors of reserved file system's region */
    uint16_t num_of_res_sec = (uint16_t)(fs->buffer[FAT_BPB_RsvdSecCnt+1]<<8) |
        fs->buffer[FAT_BPB_RsvdSecCnt];
    /* Read FAT's startaddesses and length */
    fs->fat1_begin = fs->fs_begin + num_of_res_sec;
    fs->param.num_of_fats = fs->buffer[FAT_BPB_NumFATs];
    fs->param.fat_length = (uint32_t)(fs->buffer[FAT_BPB_FATSz32+3]<<24) |
        (uint32_t)(fs->buffer[FAT_BPB_FATSz32+2]<<16) |
        (uint32_t)(fs->buffer[FAT_BPB_FATSz32+1]<<8) |
        (uint32_t)(fs->buffer[FAT_BPB_FATSz32]);
    if (fs->param.num_of_fats == 2) fs->fat2_begin = fs->fat1_begin + fs->param.fat_length;
    /* Calculate a start address of file system's data region */
    fs->data_region_begin = fs->fat1_begin + fs->param.num_of_fats * fs->param.fat_length;
    return FAT_OK;
}


/**
 * @brief Set file system temp-pointer to root directory
 * @param fs file system's descriptor-structure
 * @return none
 */
void FAT_SetPointerToRoot(FAT_Descriptor_t* fs)
{
    fs->temp.cluster = fs->data_region_begin;
}


/**
 * @brief The function finds the cluster next to [fs.temp.cluster] cluster
 * @param fs file system's descriptor-structure
 * @return
 * - FAT_OK the next cluster was found succesfully, its pointer was saved to [fs.temp.cluster]
 * - FAT_DiskError - the driver error occured, [fs.temp.cluster] not changed
 * - FAT_NotFound - there are not any next clusters, [fs.temp.cluster] not changed
 */
FAT_Status_t FAT_FindNextCluster(FAT_Descriptor_t* fs)
{
    /* Read FAT */
    uint32_t bias = (fs->temp.cluster / (512/4));
    if (SD_SingleRead(fs->card, fs->fat1_begin + bias, fs->buffer) != 0) return FAT_DiskError;
    /* Read field */
    uint32_t* ptr = (uint32_t*)fs->buffer;
    uint32_t link = ptr[fs->temp.cluster % (512/4)];
    if ((link & 0x0FFFFFFF) >= 0x0FFFFFF7) return FAT_NotFound;
    else
    {
        fs->temp.cluster = link;
        return FAT_OK;
    }
}



FAT_Status_t FAT_FindByName(FAT_Descriptor_t* fs, char* name)
{
    char name_str[11];
    /* Preparing the name string */
    uint8_t pos = 0;
    bool ready = false;
    while ((name[pos] != '\0') && !ready)
    {
        /* The point symbol has been found */
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
    /* The point symbol has not been found */
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
    /* Finding process */
    FAT_Status_t res = FAT_OK;
    while (res == FAT_OK)
    {
        for (uint8_t sector=0; sector < fs->param.sec_per_clust; sector++)
        {
            /* Read sector data */
            if (SD_SingleRead(fs->card, fs->data_region_begin+fs->temp.cluster+sector, fs->buffer) != 0) return FAT_DiskError;
            /* Try to find the name in sector */
            uint16_t entire = 0;
            while ((entire < 512) && memcmp(name_str, fs->buffer+entire, 11)) entire += 32;
            if (entire == 512) continue;
            /* The correct name has been found */
            /* Save parameters */
            fs->temp.cluster = (uint32_t)fs->buffer[entire+FAT_DIR_FstClusHI+0]<<16 |
                (uint32_t)fs->buffer[entire+FAT_DIR_FstClusHI+1]<<24 |
                (uint32_t)fs->buffer[entire+FAT_DIR_FstClusLO+0] |
                (uint32_t)fs->buffer[entire+FAT_DIR_FstClusLO+1]<<8;
            fs->temp.len = (uint32_t)fs->buffer[entire+FAT_DIR_FileSize+0] |
                (uint32_t)fs->buffer[entire+FAT_DIR_FileSize+1]<<8 |
                (uint32_t)fs->buffer[entire+FAT_DIR_FileSize+2]<<16 |
                (uint32_t)fs->buffer[entire+FAT_DIR_FileSize+3]<<24;
            fs->temp.status = fs->buffer[entire+FAT_DIR_Attr];
            return FAT_OK;
        }
        res = FAT_FindNextCluster(fs);
    }
    return res;
}