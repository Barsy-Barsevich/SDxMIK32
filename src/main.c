#include "mik32_hal_spi.h"
#include "sd.h"
#include "fat.h"
#include "mik32_hal_usart.h"
#include "xprintf.h"

/*
 * Данный пример демонстрирует работу с SPI в режиме ведущего.
 * Ведущий передает и принимает от ведомого на выводе CS0 20 байт.
 *
 * Результат передачи выводится по UART0.
 * Данный пример можно использовать совместно с ведомым из примера HAL_SPI_Slave.
 */

USART_HandleTypeDef husart0;

SPI_HandleTypeDef hspi0;
SD_Descriptor_t sd;
FAT_Descriptor_t fs;



void SystemClock_Config(void);
static void USART_Init();
static void GPIO_Init();
static void SPI0_Init();

int main()
{
    SystemClock_Config();

    USART_Init();

    GPIO_Init();

    SPI0_Init();
    sd.voltage = SD_Voltage_from_3_2_to_3_3;
    sd.spi = &hspi0;
    xprintf("\nResult if init: Status: %u; ", SD_Init(&sd));
    xprintf("Type: %s\n\n", sd.type == 0 ? "SDv1" : sd.type == 1 ? "SDv1" : sd.type == 2 ? "SDHC" : sd.type == 3 ? "MMC" : "NotSD");

    HAL_DelayMs(1000);

    HAL_GPIO_WritePin(GPIO_2, GPIO_PIN_7, 1);

    fs.card = &sd;
    xprintf("FAT init. Status: %u\n", FAT_Init(&fs));
    xprintf("First FAT startaddr: %u\nFirst data cluster: %u\n", fs.fat1_begin, fs.data_region_begin);

    // xprintf("Reading sector %u: Status: %u\n", 0, SD_SingleRead(&sd, fs.cluster_begin, fs.buffer));
    // xprintf("Erasing sector %u: Status: %u\n", 0, SD_SingleErase(&sd, fs.cluster_begin));
    // xprintf("Reading sector %u: Status: %u\n", 0, SD_SingleRead(&sd, fs.cluster_begin, fs.buffer));
    // fs.buffer[0] = 0x41;
    // fs.buffer[1] = 0x6E;
    // fs.buffer[2] = 0x6F;
    // fs.buffer[3] = 0x6D;
    // fs.buffer[4] = 0x61;
    // fs.buffer[5] = 0x6C;
    // fs.buffer[6] = 0x6A;
    // fs.buffer[7] = 0x61;
    // fs.buffer[8] = 0x20;
    // fs.buffer[9] = 0x20;
    // fs.buffer[10] = 0x20;
    // fs.buffer[11] = 0x08;
    // fs.buffer[32+0] = 'M';
    // fs.buffer[32+1] = 'y';
    // fs.buffer[32+2] = '_';
    // fs.buffer[32+3] = 'f';
    // fs.buffer[32+4] = 'i';
    // fs.buffer[32+5] = 'l';
    // fs.buffer[32+6] = 'e';
    // fs.buffer[32+7] = ' ';
    // fs.buffer[32+8] = 't';
    // fs.buffer[32+9] = 'x';
    // fs.buffer[32+10] = 't';
    // fs.buffer[32+11] = 0x00;
    // fs.buffer[32+20] = 0x00;
    // fs.buffer[32+21] = 0x00;
    // fs.buffer[32+26] = 0x01;
    // fs.buffer[32+27] = 0x00;
    // fs.buffer[32+28] = 0x05;
    // fs.buffer[32+29] = 0x00;
    // fs.buffer[32+30] = 0x00;
    // fs.buffer[32+31] = 0x00;
    // xprintf("Writing sector %u: Status: %u\n", 0, SD_SingleWrite(&sd, fs.cluster_begin, fs.buffer));
    // xprintf("Reading sector %u: Status: %u\n", 0, SD_SingleRead(&sd, fs.cluster_begin, fs.buffer));
    // for (uint16_t i=0; i<512; i+=16)
    // {
    //     xprintf("%04X: ", i);
    //     for (uint8_t j=0; j<16; j++)
    //     {
    //         xprintf(" %02X", fs.buffer[i+j]);
    //     }
    //     xprintf("\n");
    // }


    // xprintf("Reading sector %u: Status: %u\n", 1, SD_SingleRead(&sd, fs.cluster_begin+1, fs.buffer));
    // xprintf("Erasing sector %u: Status: %u\n", 1, SD_SingleErase(&sd, fs.cluster_begin+1));
    // xprintf("Reading sector %u: Status: %u\n", 1, SD_SingleRead(&sd, fs.cluster_begin+1, fs.buffer));
    // fs.buffer[0] = 0x31;
    // fs.buffer[1] = 0x32;
    // fs.buffer[2] = 0x33;
    // xprintf("Writing sector %u: Status: %u\n", 1, SD_SingleWrite(&sd, fs.cluster_begin+1, fs.buffer));
    // xprintf("Reading sector %u: Status: %u\n", 1, SD_SingleRead(&sd, fs.cluster_begin+1, fs.buffer));
    // for (uint16_t i=0; i<512; i+=16)
    // {
    //     xprintf("%04X: ", i);
    //     for (uint8_t j=0; j<16; j++)
    //     {
    //         xprintf(" %02X", fs.buffer[i+j]);
    //     }
    //     xprintf("\n");
    // }


    // xprintf("Reading sector %u: Status: %u\n", fs.fat1_begin, SD_SingleRead(&sd, fs.fat1_begin, fs.buffer));
    // for (uint16_t i=0; i<512; i+=16)
    // {
    //     xprintf("%04X: ", i);
    //     for (uint8_t j=0; j<16; j++)
    //     {
    //         xprintf(" %02X", fs.buffer[i+j]);
    //     }
    //     xprintf("\n");
    // }

    // xprintf("Sectors per cluster: %u\n", fs.param.sec_per_clust);
    // for (uint8_t x=0; x<30; x++)
    // {
    //     for (uint8_t y=0; y<4; y++)
    //     {
    //         xprintf("Reading sector %u: Status: %u\n", fs.data_region_begin+x*fs.param.sec_per_clust+y,
    //             SD_SingleRead(&sd, fs.data_region_begin+x*fs.param.sec_per_clust+y, fs.buffer));
    //         for (uint16_t i=0; i<512; i+=16)
    //         {
    //             xprintf("%04X: ", i);
    //             for (uint8_t j=0; j<16; j++)
    //             {
    //                 xprintf(" %02X", fs.buffer[i+j]);
    //             }
    //             xprintf("\n");
    //         }
    //     }
    // }



    // FAT_File_t file;
    // file.cluster = 20;
    // file.addr = 0;
    // file.len = 25;
    // file.fs = &fs;
    // static char buffer[32];
    // uint32_t read_data;

    // read_data = FAT_ReadFile(&file, buffer, 16);
    // xprintf("Read: %u\n", read_data);
    // for (uint8_t i=0; i<read_data; i++) xprintf("%02X ", buffer[i]);
    // xprintf("\n");
    // read_data = FAT_ReadFile(&file, buffer, 16);
    // xprintf("Read: %u\n", read_data);
    // for (uint8_t i=0; i<read_data; i++) xprintf("%02X ", buffer[i]);
    // xprintf("\n");


    // fs.temp.cluster = 0;
    // fs.temp.len = 0;
    // xprintf("\nPointer: %08X; Len: %08X\n", fs.temp.cluster, fs.temp.len);
    // // xprintf("Find by name: Status: %u\n", FAT_FindByName(&fs, "FOLDER"));
    // // xprintf("Pointer: %08X; Len: %08X\n", fs.temp.cluster, fs.temp.len);
    // xprintf("Find by path: Status: %u\n", FAT_FindByPath(&fs, "FOLDER/PODSTAVA.TXT"));
    // xprintf("Pointer: %08X; Len: %08X\n", fs.temp.cluster, fs.temp.len);


    FAT_File_t file;
    file.fs = &fs;
    xprintf("\n\nFile open: Status: %u\n", FAT_FileOpen(&file, "FOLDER/PODSTAVA.TXT"));
    //xprintf("*%u*\n", file.cluster);
    static char buffer[1000];
    uint32_t read_data = FAT_ReadFile(&file, buffer, file.len);
    xprintf("Reading file... Read %u bytes.\n", read_data);
    buffer[read_data] = '\0';
    //for (uint8_t i=0; i<read_data; i++) xprintf(" %02X", buffer[i]); 
    xprintf("Text: %s\n", buffer);





    // for (uint8_t i=0x0; i<30; i++)
    // {
    // xprintf("Reading sector %u: Status: %u\n", i, SD_SingleRead(&sd, fs.cluster_begin+i, fs.buffer));
    // for (uint16_t i=0; i<512; i+=16)
    // {
    //     xprintf("%04X: ", i);
    //     for (uint8_t j=0; j<16; j++)
    //     {
    //         xprintf(" %02X", fs.buffer[i+j]);
    //     }
    //     xprintf("\n");
    // }
    // }
}

void SystemClock_Config(void)
{
    PCC_InitTypeDef PCC_OscInit = {0};

    PCC_OscInit.OscillatorEnable = PCC_OSCILLATORTYPE_ALL;
    PCC_OscInit.FreqMon.OscillatorSystem = PCC_OSCILLATORTYPE_OSC32M;
    PCC_OscInit.FreqMon.ForceOscSys = PCC_FORCE_OSC_SYS_UNFIXED;
    PCC_OscInit.FreqMon.Force32KClk = PCC_FREQ_MONITOR_SOURCE_OSC32K;
    PCC_OscInit.AHBDivider = 0;
    PCC_OscInit.APBMDivider = 0;
    PCC_OscInit.APBPDivider = 0;
    PCC_OscInit.HSI32MCalibrationValue = 128;
    PCC_OscInit.LSI32KCalibrationValue = 128;
    PCC_OscInit.RTCClockSelection = PCC_RTC_CLOCK_SOURCE_AUTO;
    PCC_OscInit.RTCClockCPUSelection = PCC_CPU_RTC_CLOCK_SOURCE_OSC32K;
    HAL_PCC_Config(&PCC_OscInit);
}


static void USART_Init()
{
    husart0.Instance = UART_0;
    husart0.transmitting = Enable;
    husart0.receiving = Disable;
    husart0.frame = Frame_8bit;
    husart0.parity_bit = Disable;
    husart0.parity_bit_inversion = Disable;
    husart0.bit_direction = LSB_First;
    husart0.data_inversion = Disable;
    husart0.tx_inversion = Disable;
    husart0.rx_inversion = Disable;
    husart0.swap = Disable;
    husart0.lbm = Disable;
    husart0.stop_bit = StopBit_1;
    husart0.mode = Asynchronous_Mode;
    husart0.xck_mode = XCK_Mode3;
    husart0.last_byte_clock = Disable;
    husart0.overwrite = Disable;
    husart0.rts_mode = AlwaysEnable_mode;
    husart0.dma_tx_request = Disable;
    husart0.dma_rx_request = Disable;
    husart0.channel_mode = Duplex_Mode;
    husart0.tx_break_mode = Disable;
    husart0.Interrupt.ctsie = Disable;
    husart0.Interrupt.eie = Disable;
    husart0.Interrupt.idleie = Disable;
    husart0.Interrupt.lbdie = Disable;
    husart0.Interrupt.peie = Disable;
    husart0.Interrupt.rxneie = Disable;
    husart0.Interrupt.tcie = Disable;
    husart0.Interrupt.txeie = Disable;
    husart0.Modem.rts = Disable; //out
    husart0.Modem.cts = Disable; //in
    husart0.Modem.dtr = Disable; //out
    husart0.Modem.dcd = Disable; //in
    husart0.Modem.dsr = Disable; //in
    husart0.Modem.ri = Disable;  //in
    husart0.Modem.ddis = Disable;//out
    husart0.baudrate = 1000000;
    HAL_USART_Init(&husart0);
}


static void GPIO_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_PCC_GPIO_0_CLK_ENABLE();
    __HAL_PCC_GPIO_1_CLK_ENABLE();
    __HAL_PCC_GPIO_2_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_OUTPUT;
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_NONE;
    HAL_GPIO_Init(GPIO_2, &GPIO_InitStruct);
}


static void SPI0_Init()
{
    hspi0.Instance = SPI_0;
    /* Режим SPI */
    hspi0.Init.SPI_Mode = HAL_SPI_MODE_MASTER;
    /* Настройки */
    hspi0.Init.CLKPhase = SPI_PHASE_ON;
    hspi0.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi0.Init.ThresholdTX = 4;
    /* Настройки для ведущего */
    hspi0.Init.BaudRateDiv = SPI_BAUDRATE_DIV256;
    hspi0.Init.Decoder = SPI_DECODER_NONE;
    hspi0.Init.ManualCS = SPI_MANUALCS_ON;
    hspi0.Init.ChipSelect = SPI_CS_0;
    if (HAL_SPI_Init(&hspi0) != HAL_OK)
    {
        xprintf("SPI_Init_Error\n");
    }
}