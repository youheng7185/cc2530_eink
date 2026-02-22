#include <cc2530.h>
#include <stdint.h>
#include "uart.h"
#include "uart_rx.h"
#include "GxGDEW0213Z16.h"

__xdata uint8_t framebuffer[FRAMEBUFFER_SIZE];

/* -----------------------------------------------------------------------
 * Blocking receive one byte over UART1
 * ----------------------------------------------------------------------- */
uint8_t uart_getc(void)
{
    uint8_t b;
    while (!RX_BYTE);   /* SBIT — waits for bit 2 of U1CSR */
    b = U1DBUF;         /* reading clears RX_BYTE automatically */
    return b;
}

/* -----------------------------------------------------------------------
 * Receive N bytes and store into a buffer
 * ----------------------------------------------------------------------- */
void uart_read_bytes(__xdata uint8_t *buf, uint16_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++) {
        buf[i] = uart_getc();
    }
}

/* -----------------------------------------------------------------------
 * Protocol commands
 * ----------------------------------------------------------------------- */
#define CMD_WRITE_SCREEN 0x13
#define CMD_CLEAR_SCREEN 0x31

/* -----------------------------------------------------------------------
 * Wait for a command byte, then receive the framebuffer if needed
 * ----------------------------------------------------------------------- */
void uart_process_command(void)
{
    uint8_t cmd = uart_getc();

    switch(cmd)
    {
        case CMD_WRITE_SCREEN:
            /* Receive full framebuffer */
            uart_read_bytes(framebuffer, FRAMEBUFFER_SIZE);
            uart_printf("Received framebuffer (%u bytes)\n", FRAMEBUFFER_SIZE);
            EPD_SendFrame(framebuffer);
            break;

        case CMD_CLEAR_SCREEN:
            // clear screen
            EPD_Clear();
            uart_printf("Framebuffer cleared\n");
            break;

        default:
            uart_printf("Unknown command 0x%02X\n", cmd);
            break;
    }
}
