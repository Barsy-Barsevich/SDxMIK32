#include "mik32_hal_spi.h"
#include "sd.h"
#include "fat.h"
#include "mik32_hal_usart.h"
#include "xprintf.h"

/*
 * Данный пример демонстрирует работу драйвера карт SD и
 * файловой системы, совместимой с файловой системой FAT32
 */

#define READ_EXAMPLE
#define WRITE_EXAMPLE


USART_HandleTypeDef husart0;
FAT_Descriptor_t fs;


void SystemClock_Config(void);
static void USART_Init();

int main()
{
    SystemClock_Config();

    USART_Init();

    xprintf("\n\n*** Start ***\n");

    /* Инициализация */
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

#ifdef READ_EXAMPLE
    xprintf("\nReading file example\n");
    FAT_File_t read_file;
    res = FAT_FileOpen(&read_file, &fs, "TESTS/READ.TXT", 'R');
    xprintf("TESTS/READ.TXT file open status: %d\n", res);
    if (res != FAT_OK)
    {
        while (1)
        {
            xprintf("Error occured with file TESTS/READ.TXT, status %u\n", res);
            HAL_DelayMs(5000);
        }
    }
    static char read_buffer[1000];
    if (read_file.len > 1000-1)
    {
        xprintf("File length is more than the buffer length\n");
    }
    else
    {
        uint16_t bytes_read = FAT_ReadFile(&read_file, read_buffer, read_file.len);
        /* Вставить символ возврата каретки для корректной печати */
        read_buffer[bytes_read] = '\0';
        xprintf("Text:\n%s\n", read_buffer);
    }
    xprintf("Close status: %d\n", FAT_FileClose(&read_file));
#endif
#ifdef WRITE_EXAMPLE
    xprintf("\nWriting file example\n");
    FAT_File_t write_file;
    res = FAT_FileOpen(&write_file, &fs, "TESTS/WRITE1.TXT", 'W');
    xprintf("TESTS/WRITE1.TXT file open status: %d\n", res);
    if (res != FAT_OK)
    {
        while (1)
        {
            xprintf("Error occured with file TESTS/WRITE1.TXT, status %u\n", res);
            HAL_DelayMs(5000);
        }
    }
    char str[] = "Writing string to file\n";
    xprintf("Wrote bytes: %d\n", FAT_WriteFile(&write_file, str, strlen(str)-1));
    xprintf("Close status: %d\n", FAT_FileClose(&write_file));
#endif
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
    husart0.baudrate = 115200;
    HAL_USART_Init(&husart0);
}