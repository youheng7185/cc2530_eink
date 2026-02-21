/*****************************************************************************
* | File      	:   EPD_2in13b_V3.c
* | Author      :   Waveshare team
* | Function    :   2.13inch e-paper b V3
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2020-04-13
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
/******************************************************************************
 * EPD_2in13b_V3.c — fixed for CC2530
 *
 * Key fixes vs original Waveshare code:
 *   1. ReadBusy polarity corrected — HIGH=busy on UC8151/IL0373
 *   2. WDT_FEED() added in busy wait — refresh takes 2-3s, WD timeout is 1s
 *   3. Display/Clear loops use uint16_t not UWORD to avoid SDCC issues
 *   4. Image pointers are __code or __xdata — not plain pointer (8051 spaces)
 ******************************************************************************/
#include "EPD_2in13b_V3.h"
#include "spi.h"
#include "hal_delay.h"
#include "epd_busy.h"
#include "wdt.h"
#include "uart.h"

/* -----------------------------------------------------------------------
 * Reset
 * ----------------------------------------------------------------------- */
static void EPD_2IN13B_V3_Reset(void)
{
    SPI_CS_HIGH();
    SPI_RST_HIGH();  HAL_Delay(100);
    SPI_RST_LOW();   HAL_Delay(2);
    SPI_RST_HIGH();  HAL_Delay(10);
}

/* -----------------------------------------------------------------------
 * Send command (DC low)
 * ----------------------------------------------------------------------- */
static void EPD_2IN13B_V3_SendCommand(uint8_t reg)
{
    SPI_DC_LOW();
    SPI_CS_LOW();
    DEV_SPI_WriteByte(reg);
    SPI_CS_HIGH();
}

/* -----------------------------------------------------------------------
 * Send data (DC high)
 * ----------------------------------------------------------------------- */
static void EPD_2IN13B_V3_SendData(uint8_t data)
{
    SPI_DC_HIGH();
    SPI_CS_LOW();
    DEV_SPI_WriteByte(data);
    SPI_CS_HIGH();
}

/* -----------------------------------------------------------------------
 * Wait until BUSY pin goes LOW (ready)
 *
 * UC8151/IL0373: HIGH = busy, LOW = ready
 *
 * Original Waveshare code had this inverted:
 *   busy = !(busy & 0x01)  → was waiting while LOW, exiting when HIGH — WRONG
 *
 * Fixed: wait while EPD_Busy() returns 1 (HIGH = busy)
 *
 * Also feeds WDT — full refresh holds BUSY high for ~2-3 seconds,
 * which would trigger the 1s watchdog reset without feeding.
 * ----------------------------------------------------------------------- */
void EPD_2IN13B_V3_ReadBusy(void)
{
    uint16_t timeout = 0;
    EPD_2IN13B_V3_SendCommand(0x71);
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

/* -----------------------------------------------------------------------
 * Turn on display (trigger refresh)
 * ----------------------------------------------------------------------- */
static void EPD_2IN13B_V3_TurnOnDisplay(void)
{
    EPD_2IN13B_V3_SendCommand(0x12);   /* display refresh */
    HAL_Delay(100);
    EPD_2IN13B_V3_ReadBusy();
}

/* -----------------------------------------------------------------------
 * Init
 * ----------------------------------------------------------------------- */
// both sequence also works
void EPD_2IN13B_V3_Init(void)
{
    EPD_2IN13B_V3_Reset();
    HAL_Delay(10);

    // /* Booster soft start — missing from previous init, required */
    // EPD_2IN13B_V3_SendCommand(0x06);
    // EPD_2IN13B_V3_SendData(0x17);
    // EPD_2IN13B_V3_SendData(0x17);
    // EPD_2IN13B_V3_SendData(0x17);

    // /* Power on */
    // EPD_2IN13B_V3_SendCommand(0x04);
    // EPD_2IN13B_V3_ReadBusy();

    // /* Panel Setting
    //  * 0x8F = LUT from OTP, B/W mode, scan up, shift right, booster on
    //  * (GxEPD uses 0x8F for this exact panel GDEW0213Z16) */
    // EPD_2IN13B_V3_SendCommand(0x00);
    // EPD_2IN13B_V3_SendData(0x8F);

    // /* VCOM and data interval — 0x37 per GxEPD reference */
    // EPD_2IN13B_V3_SendCommand(0x50);
    // EPD_2IN13B_V3_SendData(0x37);

    // /* Resolution: 104 x 212 */
    // EPD_2IN13B_V3_SendCommand(0x61);
    // EPD_2IN13B_V3_SendData(0x68);
    // EPD_2IN13B_V3_SendData(0x00);
    // EPD_2IN13B_V3_SendData(0xD4);


    // EPD_2IN13B_V3_SendCommand(0x04);  
    // EPD_2IN13B_V3_ReadBusy();//waiting for the electronic paper IC to release the idle signal

    // EPD_2IN13B_V3_SendCommand(0x00);//panel setting
    // EPD_2IN13B_V3_SendData(0x0f);//LUT from OTP，128x296
    // EPD_2IN13B_V3_SendData(0x89);//Temperature sensor, boost and other related timing settings

    // EPD_2IN13B_V3_SendCommand(0x61);//resolution setting
    // EPD_2IN13B_V3_SendData (0x68);
    // EPD_2IN13B_V3_SendData (0x00);
    // EPD_2IN13B_V3_SendData (0xD4);

    // EPD_2IN13B_V3_SendCommand(0X50);//VCOM AND DATA INTERVAL SETTING
    // EPD_2IN13B_V3_SendData(0x77);//WBmode:VBDF 17|D7 VBDW 97 VBDB 57
    //                              //WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7;    


}

/* -----------------------------------------------------------------------
 * Clear — all white
 * ----------------------------------------------------------------------- */
void EPD_2IN13B_V3_Clear(void)
{
    uint16_t i, j;
    uint16_t w = (EPD_2IN13B_V3_WIDTH % 8 == 0) ?
                  EPD_2IN13B_V3_WIDTH / 8 :
                  EPD_2IN13B_V3_WIDTH / 8 + 1;
    uint16_t h = EPD_2IN13B_V3_HEIGHT;

    /* black plane — all white (0xFF) */
    EPD_2IN13B_V3_SendCommand(0x10);
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) {
            EPD_2IN13B_V3_SendData(0xFF);
            WDT_FEED();
        }

    /* red plane — all blank (0xFF) */
    EPD_2IN13B_V3_SendCommand(0x13);
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) {
            EPD_2IN13B_V3_SendData(0xFF);
            WDT_FEED();
        }

    EPD_2IN13B_V3_TurnOnDisplay();
}

/* -----------------------------------------------------------------------
 * Display image
 * blackimage / ryimage must be __code or __xdata pointers
 * ----------------------------------------------------------------------- */
void EPD_2IN13B_V3_Display(const uint8_t *blackimage,
                            const uint8_t *ryimage)
{
    uint16_t i, j;
    uint16_t w = (EPD_2IN13B_V3_WIDTH % 8 == 0) ?
                  EPD_2IN13B_V3_WIDTH / 8 :
                  EPD_2IN13B_V3_WIDTH / 8 + 1;
    uint16_t h = EPD_2IN13B_V3_HEIGHT;

    EPD_2IN13B_V3_SendCommand(0x10);
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) {
            EPD_2IN13B_V3_SendData(blackimage[i + j * w]);
            WDT_FEED();
        }

    EPD_2IN13B_V3_SendCommand(0x13);
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) {
            EPD_2IN13B_V3_SendData(ryimage[i + j * w]);
            WDT_FEED();
        }

    EPD_2IN13B_V3_TurnOnDisplay();
}

/* -----------------------------------------------------------------------
 * Sleep
 * ----------------------------------------------------------------------- */
void EPD_2IN13B_V3_Sleep(void)
{
    EPD_2IN13B_V3_SendCommand(0x50);
    EPD_2IN13B_V3_SendData(0xF7);

    EPD_2IN13B_V3_SendCommand(0x02);   /* power off */
    EPD_2IN13B_V3_ReadBusy();

    EPD_2IN13B_V3_SendCommand(0x07);   /* deep sleep */
    EPD_2IN13B_V3_SendData(0xA5);
}