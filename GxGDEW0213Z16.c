#include "GxGDEW0213Z16.h"
#include "spi.h"
#include "hal_delay.h"
#include "epd_busy.h"
#include "wdt.h"
#include "uart.h"

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
    SendData(0x8F);

    // VCOM and data interval
    SendCommand(0x50);
    SendData(0x37);

    // Resolution setting (104 x 212)
    SendCommand(0x61);
    SendData(0x68);   // 104
    SendData(0x00);
    SendData(0xD4);   // 212
}

#define BUFFER_SIZE 2756 // 104*212 / 8

void EPD_Clear(void)
{
    EPD_Init();   // wake + init

    SendCommand(0x10);
    for (uint32_t i = 0; i < BUFFER_SIZE * 2; i++)
        SendData(0xFF);

    SendCommand(0x13);
    for (uint32_t i = 0; i < BUFFER_SIZE; i++)
        SendData(0xFF);

    SendCommand(0x12);   // display refresh
    WaitBusy();

    // Sleep
    SendCommand(0x02);
    WaitBusy();
    SendCommand(0x07);
    SendData(0xA5);
}