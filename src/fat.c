#include "fat.h"


FAT_Status_enum FAT_Init(FAT_Descriptor_t* local)
{
    /* Read Master boot record */
    if (SD_SingleRead(local->card, 0, local->buffer) != 0) return FAT_DiskError;
    /* Read LBAs */
    uint8_t counter = 0;
    uint8_t type_code;
    uint8_t* ptr = local->buffer + FAT_MBR_Partition0;
    while (counter < 4)
    {
        type_code = ptr[FAT_Partition_TypeCode];
        if ((type_code == 0x0B) || (type_code == 0x0C)) break;
        counter += 1;
        ptr += FAT_MBR_Partition_Length;
    }
    if (counter == 4) return FAT_DiskNForm;
    uint32_t lba_startaddr = (uint32_t)(ptr[FAT_Partition_LBA_Begin+3]<<24) |
        (uint32_t)(ptr[FAT_Partition_LBA_Begin+2]<<16) |
        (uint32_t)(ptr[FAT_Partition_LBA_Begin+1]<<8) |
        (uint32_t)ptr[FAT_Partition_LBA_Begin];
    if (SD_SingleRead(local->card, lba_startaddr, local->buffer) != 0) return FAT_DiskError;
    local->sectors_per_cluster = local->buffer[FAT_BPB_SecPerClus];
    uint16_t num_of_res_sec = (uint16_t)(local->buffer[FAT_BPB_RsvdSecCnt+1]<<8) |
        local->buffer[FAT_BPB_RsvdSecCnt];
    local->fat_begin = lba_startaddr + num_of_res_sec;
    local->num_of_fats = local->buffer[FAT_BPB_NumFATs];
    local->fat_length = (uint32_t)(local->buffer[FAT_BPB_FATSz32+3]<<24) |
        (uint32_t)(local->buffer[FAT_BPB_FATSz32+2]<<16) |
        (uint32_t)(local->buffer[FAT_BPB_FATSz32+1]<<8) |
        (uint32_t)(local->buffer[FAT_BPB_FATSz32]);
    local->cluster_begin = local->fat_begin + local->num_of_fats * local->fat_length;
    return FAT_OK;
}