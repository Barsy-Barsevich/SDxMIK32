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

FAT_Descriptor_t fs;



void SystemClock_Config(void);
static void USART_Init();

void PrintFATs();
void PrintRoot();
void ReadSector(uint32_t num);


int main()
{
    SystemClock_Config();

    USART_Init();

    xprintf("Start\n");

    FAT_Status_t res;
    res = FAT_Init(&fs);
    xprintf("FS initialization: %s", res==FAT_OK ? "ok\n" : "failed, ");
    if (res != FAT_OK)
    {
        switch (res)
        {
            case FAT_DiskError: xprintf("disk error"); break;
            case FAT_DiskNForm: xprintf("disk not mount"); break;
            default: xprintf("unknown error"); break;
        }
        xprintf("\n");
        while(1);
    }
    // xprintf("FS startaddr: %u\n", fs.fs_begin);
    // xprintf("First FAT1 startaddr: %u\n", fs.fat1_begin);
    // xprintf("First FAT2 startaddr: %u\n", fs.fat2_begin);
    // xprintf("First data cluster: %u\n", fs.data_region_begin);
    // xprintf("FAT length: %u\n", fs.param.fat_length);
    // xprintf("Num of FATs: %u\n", fs.param.num_of_fats);
    // xprintf("Sectors per cluster: %u\n", fs.param.sec_per_clust);


    FAT_File_t file;

    xprintf("Open_Status: %u\n", FAT_FileOpen(&file, &fs, "TESTS/WRITE1.TXT", 'W'));
    xprintf("Write_Status: %u\n", FAT_WriteFile(&file, "Raz, dva, tri - podstava ne pridi", 33));
    xprintf("Close_Status: %u\n", FAT_FileClose(&file));

    //PrintRoot();

    while(1);
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