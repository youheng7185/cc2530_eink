#include "GxGDEW0213Z16.h"
#include "spi.h"
#include "hal_delay.h"
#include "epd_busy.h"
#include "wdt.h"
#include "uart.h"
#include "hello.h"

static void SendCommand(uint8_t reg)
{
    SPI_DC_LOW();
    SPI_CS_LOW();
    DEV_SPI_WriteByte(reg);
    SPI_CS_HIGH();
}

/* -----------------------------------------------------------------------
 * Send data (DC high)
 * ----------------------------------------------------------------------- */
static void SendData(uint8_t data)
{
    SPI_DC_HIGH();
    SPI_CS_LOW();
    DEV_SPI_WriteByte(data);
    SPI_CS_HIGH();
}

void WaitBusy(void)
{
    uint16_t timeout = 0;
    SendCommand(0x71);
    uart_printf("BUSY pin = %u\n", (uint16_t)EPD_Busy());
    while (EPD_Busy()) {
        WDT_FEED();
        HAL_Delay(10);
        timeout++;
        if (timeout % 100 == 0)   /* print every 1 second */
            uart_printf("still busy... %u00ms pin=%u\n",
                        timeout / 100, (uint16_t)EPD_Busy());
        if (timeout >= 1000) {     /* 10 second hard timeout */
            uart_printf("BUSY timeout! giving up\n");
            break;
        }
    }
    uart_printf("BUSY done, pin = %u\n", (uint16_t)EPD_Busy());
    HAL_Delay(200);
}

void EPD_Init(void)
{
    // Hardware reset
    SPI_RST_LOW();
    HAL_Delay(10);
    SPI_RST_HIGH();
    HAL_Delay(10);

    // Booster soft start
    SendCommand(0x06);
    SendData(0x17);
    SendData(0x17);
    SendData(0x17);

    // Power on
    SendCommand(0x04);
    WaitBusy();

    // Panel setting
    SendCommand(0x00);
    SendData(0x0F);
    
    // Resolution setting (104 x 212)
    SendCommand(0x61);
    SendData(0x68);   // 104
    SendData(0x00);
    SendData(0xD4);   // 212

    // VCOM and data interval
    SendCommand(0x50);
    SendData(0x77);

}

void EPD_Refresh(void) {
    HAL_Delay(100);
    SendCommand(0x12);
    HAL_Delay(100);
    WaitBusy();
}

#define BUFFER_SIZE 2756 // 104*212 / 8

void EPD_Clear(void)
{
    SendCommand(0x10);
    for (uint32_t i = 0; i < BUFFER_SIZE; i++)
        SendData(0x00); // seems like useless

    SendCommand(0x13);
    for (uint32_t i = 0; i < BUFFER_SIZE; i++)
        SendData(0xAA); // 0xff will show grey, 0x00 will show white

    EPD_Refresh();

    // Sleep
    SendCommand(0x02);
    WaitBusy();
}

void EPD_Test(void) 
{
    // SendCommand(0x10);
    // for (uint32_t i = 0; i < BUFFER_SIZE; i++)
    //     SendData(~black[i]);

    SendCommand(0x13);
    for (uint32_t i = 0; i < BUFFER_SIZE; i++)
        SendData(~black[i]);

    EPD_Refresh();

    // Sleep
    SendCommand(0x02);
    WaitBusy();
}