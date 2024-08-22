

#include "sd.h"


/* Master boot record */
#define FAT_MBR_Partition0          0x1BE
#define FAT_MBR_Partition1          0x1CE
#define FAT_MBR_Partition2          0x1DE
#define FAT_MBR_Partition3          0x1EE
#define FAT_MBR_Signature           0x1FE
#define FAT_MBR_Partition_Length    16
/* Partition */
#define FAT_Partition_BootFlag      0
#define FAT_Partition_CHS_Begin     1
#define FAT_Partition_TypeCode      4
#define FAT_Partition_CHS_End       5
#define FAT_Partition_LBA_Begin     8
#define FAT_Partition_NumOfSec      12
/* File system's LBA */
#define FAT_BPB_BytsPerSec          0x0B
#define FAT_BPB_SecPerClus          0x0D
#define FAT_BPB_RsvdSecCnt          0x0E
#define FAT_BPB_NumFATs             0x10
#define FAT_BPB_FATSz32             0x24
#define FAT_BPB_RootClus            0x2C









#define FAT_DIR_Name            0
#define FAT_DIR_Attr            11
#define FAT_DIR_NTRes           12
#define FAT_DIR_CrtTimeTenth    13
#define FAT_DIR_CrtTime         14
#define FAT_DIR_CrtDate         16
#define FAT_DIR_LstAccDate      18
#define FAT_DIR_FstClusHI       20
#define FAT_DIR_WrtTime         22
#define FAT_DIR_WrtDate         24
#define FAT_DIR_FstClusLO       26
#define FAT_DIR_FileSize        28
#define FAT_ATTR_READ_ONLY      0x01
#define FAT_ATTR_HIDDEN         0x02
#define FAT_ATTR_SYSTEM         0x04
#define FAT_ATTR_VOLUME_ID      0x08
#define FAT_ATTR_DIRECTORY      0x10
#define FAT_ATTR_ARCHIVE        0x20


typedef enum
{
    FAT_OK = 0,
    FAT_DiskError = 1,
    /* Disk not formatted for FAT32 */
    FAT_DiskNForm = 2
} FAT_Status_enum;


typedef struct 
{
    SD_Descriptor_t* card;
    uint8_t buffer[512];
    uint32_t fat_begin;
    uint32_t cluster_begin;
    uint8_t sectors_per_cluster;
    uint8_t num_of_fats;
    uint32_t fat_length;
} FAT_Descriptor_t;





FAT_Status_enum FAT_Init(FAT_Descriptor_t* local);