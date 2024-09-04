#include "mik32_hal_spi.h"
#include "sd.h"
#include "fat.h"
#include "mik32_hal_usart.h"
#include "xprintf.h"

#include "text.h"

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

    xprintf("Start\n");

    SPI0_Init();
    sd.voltage = SD_Voltage_from_3_2_to_3_3;
    sd.spi = &hspi0;
    xprintf("\nResult if init: Status: %u; ", SD_Init(&sd));
    xprintf("Type: %s\n\n", sd.type == 0 ? "SDv1" : sd.type == 1 ? "SDv1" : sd.type == 2 ? "SDHC" : sd.type == 3 ? "MMC" : "NotSD");
    HAL_DelayMs(1000);
    HAL_GPIO_WritePin(GPIO_2, GPIO_PIN_7, 1);

    fs.card = &sd;
    xprintf("FAT init. Status: %u\n", FAT_Init(&fs));
    xprintf("FS startaddr: %u\n", fs.fs_begin);
    xprintf("First FAT1 startaddr: %u\n", fs.fat1_begin);
    xprintf("First FAT2 startaddr: %u\n", fs.fat2_begin);
    xprintf("First data cluster: %u\n", fs.data_region_begin);
    xprintf("FAT length: %u\n", fs.param.fat_length);
    xprintf("Num of FATs: %u\n", fs.param.num_of_fats);
    xprintf("Sectors per cluster: %u\n", fs.param.sec_per_clust);
    

    xprintf("read sector: status: %u\n", SD_SingleRead(fs.card, fs.data_region_begin, fs.buffer));
    // xprintf("Reading sector %u: Status: %u\n", i, SD_SingleRead(&sd, fs.cluster_begin+i, fs.buffer));
    for (uint16_t i=0; i<512; i+=16)
    {
        xprintf("%04X: ", i);
        for (uint8_t j=0; j<16; j++)
        {
            xprintf(" %02X", fs.buffer[i+j]);
        }
        xprintf("\n");
    }

    uint8_t status;
    FAT_File_t file;
    file.fs = &fs;


    static char str[] = "Kazhdyi den novaja podstava";
    static char text[1000];

    /* Open file */
    status = FAT_FileOpen(&file, "PODSTAVA.TXT", 'W');
    xprintf("\nFile open: Status: %u\n", status);
    /* Write to file */
    uint32_t wrote_data = FAT_WriteFile(&file, str, strlen(str));
    xprintf("Wrote %u bytes\n", wrote_data);
    /* Close file */
    status  = FAT_FileClose(&file);
    xprintf("File close: Status: %u\n", status);

    /* Open file */
    status = FAT_FileOpen(&file, "PODSTAVA.TXT", 'R');
    xprintf("\n\nFile open: Status: %u\n", status);
    /* Read file */
    uint32_t read_data = FAT_ReadFile(&file, text, 1000);
    xprintf("Reading data... Read %u bytes.\n", read_data);
    /* Print data */
    text[read_data] = '\0';
    xprintf("Text: %s\n", text);



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