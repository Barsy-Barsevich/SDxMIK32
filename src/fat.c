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


/**
 * @brief Find the number of 1st cluster of file/subdirectory in the directory
 * @param fs pointer to file system's structure-descriptor
 * @param name string of name. The last byte should be '\0' or '/'.
 * If the name contains a point '.' symbol, it cannot contain more than 8 meanung
 * symbols before point and more than 3 meaning symbols after point. Else, the
 * name cannot contain more than 8 meaning symbols
 * @return 
 */
FAT_Status_t FAT_FindByName(FAT_Descriptor_t* fs, char* name)
{
    char name_str[11];
    /* Preparing the name string */
    uint8_t pos = 0;
    bool ready = false;
    while ((name[pos] != '\0') && (name[pos] != '/') && !ready)
    {
        /* The point symbol has been found */
        if (name[pos] == '.')
        {
            if (pos > 8) return FAT_Error;
            memcpy(name_str, name, pos);
            for (uint8_t i=pos; i<8; i++) name_str[i] = 0x20;
            uint8_t i=0;
            while ((name[pos+1+i] != '\0') && (name[pos+1+i] != '/'))
            {
                if (pos+i > 11) return FAT_Error;
                name_str[8+i] = name[pos+1+i];
                i += 1;
            }
            for ((void)i; i<11-8; i++) name_str[i] = 0x20;
            ready = true;
        }
        pos += 1;
    }
    /* The point symbol has not been found */
    if (!ready)
    {
        uint8_t i=0;
        while ((name[i] != '\0') && (name[i] != '/'))
        {
            if (i > 8) return FAT_Error;
            name_str[i] = name[i];
            i += 1;
        }
        for ((void)i; i<11; i++) name_str[i] = 0x20;
    }
    //xprintf("\n*%s*\n", name_str);
    /* Finding process */
    FAT_Status_t res = FAT_OK;
    while (res == FAT_OK)
    {
        uint32_t sector;
        for (uint8_t sec=0; sec < fs->param.sec_per_clust; sec++)
        {
            /* Read sector data */
            sector = fs->data_region_begin + fs->temp.cluster * fs->param.sec_per_clust + sec;
            if (SD_SingleRead(fs->card, sector, fs->buffer) != SD_OK) return 49/*FAT_DiskError*/;
            //xprintf("\n*%u*\n", cluster);
            /* Try to find the name in sector */
            uint16_t entire = 0;
            while ((entire < 512) && memcmp(name_str, fs->buffer+entire, 11)) entire += 32;
            if (entire == 512) continue;
            /* The correct name has been found */
            /* Save parameters */
            fs->temp.entire_in_dir_clust = entire;
            fs->temp.dir_sector = fs->temp.cluster * fs->param.sec_per_clust + sec;
            fs->temp.cluster = (uint32_t)fs->buffer[entire+FAT_DIR_FstClusHI+0]<<16 |
                (uint32_t)fs->buffer[entire+FAT_DIR_FstClusHI+1]<<24 |
                (uint32_t)fs->buffer[entire+FAT_DIR_FstClusLO+0] |
                (uint32_t)fs->buffer[entire+FAT_DIR_FstClusLO+1]<<8;
            fs->temp.cluster -= 2;
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


FAT_Status_t FAT_FindByPath(FAT_Descriptor_t* fs, char* path)
{
    /* calculate number of '/' symbols */
    uint8_t descend_number = 0;
    uint8_t i = 0;
    while (path[i] != '\0')
    {
        if (path[i] == '/') descend_number += 1;
        i += 1;
    }
    /* Descend into directories and files */
    FAT_Status_t res;
    res = FAT_FindByName(fs, path);
    if (res != FAT_OK) return res;
    char* ptr = path;
    for (uint8_t k=0; k<descend_number; k++)
    {
        while (*ptr != '/') ptr += 1;
        ptr += 1;
        //xprintf("\n*%s*\n", ptr);
        res = FAT_FindByName(fs, ptr);
        if (res != FAT_OK) return res;
    }
    return FAT_OK;
}


FAT_Status_t FAT_FindFreeCluster(FAT_Descriptor_t* fs, uint32_t cluster, uint32_t* new_cluster)
{
    /* Мониторим FAT */
    uint32_t x = 0;
    uint32_t* ptr;
    uint32_t clust = 0;
    do
    {
        if (SD_SingleRead(fs->card, fs->fat1_begin + x, fs->buffer) != SD_OK) return FAT_DiskError;
        x += 1;
        do
        {
            ptr = fs->buffer + clust;
            clust += 4;
        } while ((*ptr != 0) && (clust % 512 != 0));
    } while ((*ptr != 0) && (x < fs->param.fat_length));
    if (x >= fs->param.fat_length) return FAT_NoFreeSpace;
    /* Save number of new cluster */
    *new_cluster = clust;
    /* Find sector of FAT containing previous cluster link */
    if (SD_SingleRead(fs->card, fs->fat1_begin + (cluster / (512/4)), fs->buffer) != SD_OK) return FAT_DiskError;
    ptr = (uint32_t*)(fs->buffer + cluster % (512/4) * 4);
    *ptr = clust;
    if (SD_SingleErase(fs->card, fs->fat1_begin + (cluster / (512/4))) != SD_OK) return FAT_DiskError;
    if (SD_SingleWrite(fs->card, fs->fat1_begin + (cluster / (512/4)), fs->buffer) != 0) return FAT_DiskError;
    if (SD_SingleErase(fs->card, fs->fat2_begin + (cluster / (512/4))) != SD_OK) return FAT_DiskError;
    if (SD_SingleWrite(fs->card, fs->fat2_begin + (cluster / (512/4)), fs->buffer) != 0) return FAT_DiskError;
}




FAT_Status_t FAT_FileOpen(FAT_File_t* file, char* name, char modificator)
{
    /* Set pointer to root directory */
    FAT_SetPointerToRoot(file->fs);
    /* Find 1st cluster of file by path */
    FAT_Status_t res;
    res = FAT_FindByPath(file->fs, name);
    if (res != FAT_OK) return res;
    /* File start settings */
    file->cluster = file->fs->temp.cluster;
    file->dir_sector = file->fs->temp.dir_sector;
    file->entire_in_dir_clust = file->fs->temp.entire_in_dir_clust;
    file->status = file->fs->temp.status;
    file->modificator = modificator;
    switch (modificator)
    {
        case 'R':
            file->addr = 0;
            file->len = file->fs->temp.len;
            break;
        case 'W':
            file->addr = 0;
            file->len = 0;
            break;
    }
    return FAT_OK;
}


FAT_Status_t FAT_FileClose(FAT_File_t* file)
{
    if (file->modificator == 'W')
    {
        /* Write new file length */
        uint32_t sector = file->fs->data_region_begin + file->dir_sector;
        if (SD_SingleRead(file->fs->card, sector, file->fs->buffer) != SD_OK) return FAT_DiskError;
        uint32_t* ptr = (uint32_t*)&file->fs->buffer[file->entire_in_dir_clust + FAT_DIR_FileSize];
        *ptr = file->len;
        if (SD_SingleErase(file->fs->card, sector) != SD_OK) return FAT_DiskError;
        if (SD_SingleWrite(file->fs->card, sector, file->fs->buffer) != 0) return FAT_DiskError;
    }
    return FAT_OK;
}




/**
 * @brief Read data from the file
 * @param file pointer to file's structure-descriptor
 * @param buf buffer for data
 * @param quan number of bytes to read
 * @return number of read bytes
 */
uint32_t FAT_ReadFile(FAT_File_t* file, char* buf, uint32_t quan)
{
    uint32_t counter = 0;
    uint32_t start_addr = (file->addr - file->addr % (512 * file->fs->param.sec_per_clust));

    while ((quan > 0) && (file->len > 0))
    {
        /* Cluster */
        uint32_t new_cluster = (file->addr - start_addr) / (512 * file->fs->param.sec_per_clust);
        if (new_cluster != 0) start_addr += 512 * file->fs->param.sec_per_clust;
        file->fs->temp.cluster = file->cluster;
        while (new_cluster > 0)
        {
            FAT_FindNextCluster(file->fs);
            new_cluster -= 1;
        }
        file->cluster = file->fs->temp.cluster;
        /* Read sector data */
        uint32_t sector = file->fs->data_region_begin + file->cluster * file->fs->param.sec_per_clust +
            ((file->addr - start_addr) / 512);
        if (SD_SingleRead(file->fs->card, sector, file->fs->buffer) != 0) return counter;
        xprintf("*%u*\n", sector);
        /* Reading sector */
        uint16_t x = file->addr % 512;
        while ((x < 512) && (quan > 0) && (file->len > 0))
        {
            buf[counter] = file->fs->buffer[x];
            counter += 1;
            x += 1;
            quan -= 1;
            file->addr += 1;
            file->len -= 1;
        }
    }
    return counter;
}


uint32_t FAT_WriteFile(FAT_File_t* file, char* buf, uint32_t quan)
{
    uint32_t counter = 0;
    uint32_t clust_len = 512 * file->fs->param.sec_per_clust;
    uint32_t addr_in_cluster = file->addr % clust_len;
    if (addr_in_cluster != 0)
    {
        uint32_t sector = file->fs->data_region_begin + file->cluster *
            file->fs->param.sec_per_clust + addr_in_cluster / 512;
        if (SD_SingleRead(file->fs->card, sector, file->fs->buffer) != SD_OK) return counter;
    }
    while (quan > 0)
    {
        if (addr_in_cluster >= clust_len)
        {
            addr_in_cluster -= clust_len;
            //Make new cluster
            uint32_t value_buf;
            FAT_Status_t err = FAT_FindFreeCluster(file->fs, file->cluster, &value_buf);
            if (err != FAT_OK) return err;
            file->cluster = value_buf;
        }
        uint32_t sector = file->fs->data_region_begin + file->cluster *
            file->fs->param.sec_per_clust + addr_in_cluster / 512;
        /* Write data */
        uint16_t x = addr_in_cluster % 512;
        while ((x < 512) && (quan > 0))
        {
            file->fs->buffer[x] = buf[counter];
            counter += 1;
            x += 1;
            quan -= 1;
            file->addr += 1;
            addr_in_cluster += 1;
            file->len += 1;
        }
        if (SD_SingleErase(file->fs->card, sector) != 0) return counter;
        if (SD_SingleWrite(file->fs->card, sector, file->fs->buffer) != 0) return counter;
    }
    // /* Write new file length */
    // uint32_t sector = file->fs->data_region_begin + file->dir_sector;
    // if (SD_SingleRead(file->fs->card, sector, file->fs->buffer) != SD_OK) return counter;
    // uint32_t* ptr = &file->fs->buffer[file->entire_in_dir_clust + FAT_DIR_FileSize];
    // *ptr = file->len;
    // if (SD_SingleErase(file->fs->card, sector) != SD_OK) return counter;
    // if (SD_SingleWrite(file->fs->card, sector, file->fs->buffer) != 0) return counter;
    return counter;
}