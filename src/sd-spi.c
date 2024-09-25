#include "sd.h"

SPI_HandleTypeDef hspi0;
SD_Descriptor_t sd;

__attribute__((weak)) SD_Status_t SD_card_init(SD_Descriptor_t** card)
{
    /* SPI init */
    hspi0.Instance = SPI_0;
    hspi0.Init.SPI_Mode = HAL_SPI_MODE_MASTER;
    hspi0.Init.CLKPhase = SPI_PHASE_ON;
    hspi0.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi0.Init.ThresholdTX = 4;
    hspi0.Init.BaudRateDiv = SPI_BAUDRATE_DIV256;
    hspi0.Init.Decoder = SPI_DECODER_NONE;
    hspi0.Init.ManualCS = SPI_MANUALCS_ON;
    hspi0.Init.ChipSelect = SPI_CS_0;
    if (HAL_SPI_Init(&hspi0) != HAL_OK)
    {
        xprintf("SPI_Init_Error\n");
        return SD_CommunicationError;
    }

    /* SD card init */
    *card = &sd;
    sd.voltage = SD_Voltage_from_3_2_to_3_3;
    sd.spi = &hspi0;
    SD_Status_t res = SD_Init(&sd);
    xprintf("SD card: %s\n", res==SD_OK ? "found" : "not found");
    if (res != SD_OK) return res;
    xprintf("Type: ");
    switch (sd.type)
    {
        case SDv1: xprintf("SDv1"); break;
        case SDv2: xprintf("SDv2"); break;
        case SDHC: xprintf("SDHC"); break;
        case MMC: xprintf("MMC"); break;
        default: xprintf("Unknown");
    }
    xprintf("\n");
}


__attribute__((weak)) SD_Status_t SD_clock_increase()
{
    hspi0.Init.BaudRateDiv = SPI_BAUDRATE_DIV4;
    if (HAL_SPI_Init(&hspi0) != HAL_OK)
    {
        xprintf("SPI_Init_Error\n");
        return SD_CommunicationError;
    }
    return SD_OK;
}