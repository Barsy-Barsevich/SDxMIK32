#include "sd.h"


void SD_SendCommand(SD_Descriptor_t* local, SD_Commands_enum command, uint32_t operand, uint8_t crc, uint8_t* resp)
{
    uint8_t data = 0xFF;
    //HAL_SPI_CS_Enable(local->spi, SPI_CS_0);
    do
    {
        HAL_SPI_Exchange(local->spi, &data, resp, 1, SPI_TIMEOUT_DEFAULT);
    } while (*resp != 0xFF);
    data = (uint8_t)command;
    HAL_SPI_Exchange(local->spi, &data, resp, 1, SPI_TIMEOUT_DEFAULT);
    data = ((operand>>24) & 0xFF);
    HAL_SPI_Exchange(local->spi, &data, resp, 1, SPI_TIMEOUT_DEFAULT);
    data = ((operand>>16) & 0xFF);
    HAL_SPI_Exchange(local->spi, &data, resp, 1, SPI_TIMEOUT_DEFAULT);
    data = ((operand>>8) & 0xFF);
    HAL_SPI_Exchange(local->spi, &data, resp, 1, SPI_TIMEOUT_DEFAULT);
    data = (operand & 0xFF);
    HAL_SPI_Exchange(local->spi, &data, resp, 1, SPI_TIMEOUT_DEFAULT);
    data = crc;
    HAL_SPI_Exchange(local->spi, &data, resp, 1, SPI_TIMEOUT_DEFAULT);
    // data = 0xFF;
    // HAL_SPI_Exchange(local->spi, &data, resp, 1, SPI_TIMEOUT_DEFAULT);
    // data = 0xFF;
    // HAL_SPI_Exchange(local->spi, &data, resp, 1, SPI_TIMEOUT_DEFAULT);
    data = 0xFF;
    do
    {
        HAL_SPI_Exchange(local->spi, &data, resp, 1, SPI_TIMEOUT_DEFAULT);
    } while (*resp == 0xFF);
    
    /* R7 response */
    if ((command == CMD8) || (command == CMD58))
    {
        for (uint8_t i=0; i<4; i++)
            HAL_SPI_Exchange(local->spi, &data, resp+i+1, 1, SPI_TIMEOUT_DEFAULT);
    }

    //HAL_SPI_CS_Disable(local->spi);
    //xprintf("Com: 0x%X; R1: %08b", command, resp[0]);
    if ((command == CMD8) || (command == CMD58))
    {
        uint32_t ocr = ((uint32_t)resp[1]<<24 | (uint32_t)resp[2]<<16 |
                        (uint32_t)resp[3]<<8 | resp[4]);
        //xprintf(", extra: %032b", ocr);
    }
    //xprintf("\n");
}




SD_Status_t SD_Init(SD_Descriptor_t* local)
{
    HAL_SPI_CS_Disable(local->spi);
    HAL_DelayMs(100);
    //80 тактов на линии SCK
    uint8_t data = 0xFF;
    uint8_t dummy;
    for (uint8_t i=0; i<10; i++)
        HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);

    uint8_t resp[5];

    HAL_SPI_CS_Enable(local->spi, SPI_CS_0);

    SD_SendCommand(local, CMD0, 0, 0x95, resp);

    SD_SendCommand(local, CMD8, 0x1AA, 0x87, resp);
    
    /* It is v1 SD-card or not-SD-card */
    if (resp[0] & SD_R1_ILLEGAL_COMMAND_M)
    {
        SD_SendCommand(local, CMD58, 0, 0xFF, resp);
        uint32_t ocr = ((uint32_t)resp[1]<<24 | (uint32_t)resp[2]<<16 |
                        (uint32_t)resp[3]<<8 | resp[4]);
        if (!(local->voltage & ocr))
        {
            HAL_SPI_CS_Disable(local->spi);
            return SD_IncorrectVoltage;
        }
        if (resp[0] & SD_R1_ILLEGAL_COMMAND_M)
        {
            HAL_SPI_CS_Disable(local->spi);
            return SD_UnknownCard;
        }
        uint8_t counter = 200;

        /* Trying to send ACMD41 */
        SD_SendCommand(local, CMD55, 0, 0xFF, resp);
        SD_SendCommand(local, ACMD41, 0x40000000, 0xFF, resp);

        /* It is a MMC */
        if (resp[0] & SD_R1_ILLEGAL_COMMAND_M)
        {
            /* >74 clock cycles on SCK */
            for (uint8_t i=0; i<10; i++) 
                HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);
            /* Go from idle_mode */
            while (resp[0] & SD_R1_IDLE_STATE_M)
            {
                SD_SendCommand(local, CMD1, 0, 0xFF, resp);
                counter -= 1;
                if (counter == 0)
                {
                    HAL_SPI_CS_Disable(local->spi);
                    return SD_TimeoutError;
                }
            }
            local->type = MMC;
            HAL_SPI_CS_Disable(local->spi);
            return SD_OK;
        }
        /* It is a SDv1 */
        else
        {
            /* Go from idle_mode */
            while (resp[0] & SD_R1_IDLE_STATE_M)
            {
                SD_SendCommand(local, CMD55, 0, 0xFF, resp);
                SD_SendCommand(local, ACMD41, 0x40000000, 0xFF, resp);
                counter -= 1;
                if (counter == 0)
                {
                    HAL_SPI_CS_Disable(local->spi);
                    return SD_TimeoutError;
                }
            }
            local->type = SDv1;
            HAL_SPI_CS_Disable(local->spi);
            return SD_OK;
        }
    }

    /* It is SD v2, SDHC or SDXC card */
    else
    {
        /* check the check_pattern */
        if (resp[4] != 0xAA)
        {
            HAL_SPI_CS_Disable(local->spi);
            return SD_CommunicationError;
        }

        /* Check the card's valid voltage */
        SD_SendCommand(local, CMD58, 0, 0xFF, resp);
        uint32_t ocr = ((uint32_t)resp[1]<<24 | (uint32_t)resp[2]<<16 |
                        (uint32_t)resp[3]<<8 | resp[4]);
        if (!(local->voltage & ocr))
        {
            HAL_SPI_CS_Disable(local->spi);
            return SD_IncorrectVoltage;
        }

        /* >74 clock cycles on SCK */
            for (uint8_t i=0; i<10; i++) 
                HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);

        /* Go from idle_mode */
        uint8_t counter = 200;
        while (resp[0] & SD_R1_IDLE_STATE_M)
        {
            SD_SendCommand(local, CMD55, 0, 0xFF, resp);
            SD_SendCommand(local, ACMD41, 0x40000000, 0xFF, resp);
            counter -= 1;
            if (counter == 0)
            {
                HAL_SPI_CS_Disable(local->spi);
                return SD_TimeoutError;
            }
        }

        /* Read the CCS value */
        SD_SendCommand(local, CMD58, 0, 0xFF, resp);
        uint8_t ccs = resp[1] & 0b01000000;
        if (ccs == 0) local->type = SDv2;
        else local->type = SDHC;
        HAL_SPI_CS_Disable(local->spi);
        return SD_OK;
    }
    
    // while(1)
    // {
    //     SD_SendCommand(local, CMD55, 0, 0xFF);
    //     SD_SendCommand(local, ACMD41, 0x40000000, 0xFF);
    // }
    //     HAL_DelayMs(100);

    // while (1)
    // {
    // // HAL_SPI_CS_Enable(local->spi, SPI_CS_0);

    // // data = 0xFF;
    // // HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);
    // // data = 0x40;
    // // HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);
    // // data = 0x00;
    // // HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);
    // // data = 0x00;
    // // HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);
    // // data = 0x00;
    // // HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);
    // // data = 0x00;
    // // HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);
    // // data = 0x95;
    // // HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);
    // // data = 0xFF;
    // // HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);
    // // data = 0xFF;
    // // HAL_SPI_Exchange(local->spi, &data, &dummy, 1, SPI_TIMEOUT_DEFAULT);
    // // HAL_SPI_CS_Disable(local->spi);
    // //     xprintf("R1: %08b\n", dummy);
    // //     HAL_DelayMs(100);
    // }
}


SD_Status_t SD_SingleRead(SD_Descriptor_t* local, uint32_t addr, uint8_t* buf)
{
    uint8_t resp, dummy = 0xFF;
    HAL_SPI_CS_Enable(local->spi, SPI_CS_0);
    SD_SendCommand(local, CMD17, addr, 0xff, &resp);
    // for (uint16_t i=0; i<512; i++)
    //     HAL_SPI_Exchange(local->spi, &dummy, &resp, 1, SPI_TIMEOUT_DEFAULT);
    if (resp != 0) return resp;
    uint16_t counter = 0;
    while ((resp != 0xFE) && (resp != 0xFC))
    {
        HAL_SPI_Exchange(local->spi, &dummy, &resp, 1, SPI_TIMEOUT_DEFAULT);
        if (counter >= SD_SREAD_WAIT_ATTEMPTS) return SD_TimeoutError;
        counter += 1;
        if (resp == 0xFF) continue;
    }
    for (uint16_t i=0; i<512; i++)
    {
        HAL_SPI_Exchange(local->spi, &dummy, buf+i, 1, SPI_TIMEOUT_DEFAULT);
    }
    HAL_SPI_Exchange(local->spi, &dummy, &resp, 1, SPI_TIMEOUT_DEFAULT);
    HAL_SPI_Exchange(local->spi, &dummy, &resp, 1, SPI_TIMEOUT_DEFAULT);
    HAL_SPI_CS_Disable(local->spi);
    return SD_OK;
}


SD_Status_t SD_SingleWrite(SD_Descriptor_t* local, uint32_t addr, uint8_t* buf)
{
    uint8_t resp, dummy = 0xFF;
    HAL_SPI_CS_Enable(local->spi, SPI_CS_0);
    SD_SendCommand(local, CMD24, addr, 0xff, &resp);
    if (resp != 0) return resp;
    HAL_SPI_Exchange(local->spi, &dummy, &resp, 1, SPI_TIMEOUT_DEFAULT);
    dummy = 0xFE;
    HAL_SPI_Exchange(local->spi, &dummy, &resp, 1, SPI_TIMEOUT_DEFAULT);
    for (uint16_t i=0; i<512; i++)
    {
        HAL_SPI_Exchange(local->spi, buf+i, &resp, 1, SPI_TIMEOUT_DEFAULT);
    }
    dummy = 0xFF;
    HAL_SPI_Exchange(local->spi, &dummy, &resp, 1, SPI_TIMEOUT_DEFAULT);
    HAL_SPI_Exchange(local->spi, &dummy, &resp, 1, SPI_TIMEOUT_DEFAULT);
    HAL_SPI_Exchange(local->spi, &dummy, &resp, 1, SPI_TIMEOUT_DEFAULT);
    HAL_SPI_Exchange(local->spi, &dummy, &resp, 1, SPI_TIMEOUT_DEFAULT);
    //HAL_DelayMs(100);
    HAL_SPI_CS_Disable(local->spi);
    return SD_OK;
}


SD_Status_t SD_SingleErase(SD_Descriptor_t* local, uint32_t addr)
{
    uint8_t resp;
    HAL_SPI_CS_Enable(local->spi, SPI_CS_0);
    SD_SendCommand(local, CMD32, addr, 0xFF, &resp);
    if (resp != 0) return resp;
    SD_SendCommand(local, CMD33, addr, 0xFF, &resp);
    if (resp != 0) return resp;
    SD_SendCommand(local, CMD38, 0, 0xFF, &resp);
    HAL_SPI_CS_Disable(local->spi);
    return SD_OK;
}