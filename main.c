/*
 * CC2530 UART printf over USART1 Alt.2
 *
 * Pin mapping (from PCB reverse):
 *   P1_6 → UART TX  (USART1 Alt.2)
 *   P1_7 → UART RX  (USART1 Alt.2)
 *
 * Build:
 *   sdcc -mmcs51 --model-large --xram-size 8192 --xram-loc 0x0000 \
 *        --code-size 0x7F00 --iram-size 256 --stack-size 64 \
 *        main.c -o firmware.ihx
 *   makebin -p firmware.ihx firmware.bin
 */

#include <cc2530.h>
#include <stdint.h>
#include <stdarg.h>
#include "uart.h"
#include "wdt.h"
#include "spi.h"
#include "epd_busy.h"
#include "GxGDEW0213Z16.h"

/* -----------------------------------------------------------------------
 * Clock
 * ----------------------------------------------------------------------- */
static void clock_init(void)
{
    CLKCONCMD = 0x00;           /* 32 MHz XOSC, no divider */
    while (CLKCONSTA & 0x40);  /* wait until stable */
}

/* -----------------------------------------------------------------------
 * Main — test the UART
 * ----------------------------------------------------------------------- */
void main(void)
{
    uint8_t  i;
    uint16_t counter = 0;

    clock_init();
    wdt_init();
    uart_init();
    SPI_Init();

    EPD_Busy_Init();

    uart_puts("CC2530 UART ready\n");
    uart_printf("Chip ID : 0x%04x\n", (uint16_t)CHIPID);
    uart_printf("ClkConSta: 0x%02x\n", (uint16_t)CLKCONSTA);

    /* Stack pointer is SFR SP (0x81). SDCC initialises it to 0x3F.
     * It grows UP toward 0xFF. Past 0xFF it silently wraps into SFR
     * space and corrupts registers — no fault, just random resets.
     * Print it here so we can see how deep the call chain went. */
    uart_printf("SP after init: 0x%02x  headroom: %u bytes\n",
                (uint16_t)SP, (uint16_t)(0xFF - SP));

    uart_puts("Counting...\n");

    //EPD_test();

    EPD_Init();
    // EPD_Clear();
    EPD_Test();

    while (1)
    {
        uart_puts("hello world\n");
        counter++;

        /* print SP every 16 iters to watch for stack creep */
        if ((counter & 0x0F) == 0)
            uart_printf("[%u] SP=0x%02x headroom=%u\n",
                        counter, (uint16_t)SP, (uint16_t)(0xFF - SP));

        WDT_FEED();

        for (i = 0; i < 200; i++) {
            uint16_t j;
            for (j = 0; j < 535; j++);
            WDT_FEED();
        }
    }
}