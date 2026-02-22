/*
 * spi.c — USART0 in SPI master mode, Alt.2 pinout
 *
 * Board pins (from PCB reverse):
 *   P0_5 → EINK_CLK   (USART0 Alt.2 SCK)
 *   P0_3 → EINK_MOSI  (USART0 Alt.2 MOSI/TX)
 *   P0_4 → EINK_CS    (manual GPIO — P0_4 is MISO on Alt.2 but
 *                       e-ink is write-only so we use it as CS GPIO)
 *
 * STM32 equivalent config translated:
 *   Mode     = SPI_MODE_MASTER       → U0CSR bit5=0 (master)
 *   CPOL     = SPI_POLARITY_LOW      → U0GCR CPOL=0
 *   CPHA     = SPI_PHASE_1EDGE       → U0GCR CPHA=0  (sample on 1st edge)
 *   FirstBit = SPI_FIRSTBIT_MSB      → U0GCR ORDER=1
 *   DataSize = 8-bit                 → always 8-bit on CC2530 USART
 *   NSS      = SPI_NSS_SOFT          → manual CS GPIO
 *   Prescaler= SPI_BAUDRATEPRESCALER_64 → ~500kHz @ 32MHz (see below)
 *
 * Baud rate:
 *   F = (256 + BAUD_M) * 2^BAUD_E / 2^28 * 32MHz
 *   BAUD_E=13, BAUD_M=0 → ~1MHz  (close to STM32 @64MHz/64 = 1MHz)
 *   BAUD_E=12, BAUD_M=0 → ~500kHz (conservative, safe for first bringup)
 */

#include "spi.h"
#include <cc2530.h>

/* -----------------------------------------------------------------------
 * CS pin — P0_4 as plain GPIO
 * ----------------------------------------------------------------------- */
void SPI_CS_LOW(void)  { P0_4 = 0; }
void SPI_CS_HIGH(void) { P0_4 = 1; }

// dc pin
void SPI_DC_LOW(void)  { P0_2 = 0; }
void SPI_DC_HIGH(void) { P0_2 = 1; }

// rst pin
void SPI_RST_LOW(void)  { P1_1 = 0; }
void SPI_RST_HIGH(void) { P1_1 = 1; }

/* -----------------------------------------------------------------------
 * SPI init
 * ----------------------------------------------------------------------- */
void SPI_Init(void)
{
    /* --- Pin mux ---
     * PERCFG bit0: U0CFG  0=Alt.1, 1=Alt.2
     * Alt.2 routes USART0 to: P0_3=TX(MOSI), P0_4=RX(MISO), P0_5=SCK
     */
    //PERCFG |= 0x01;                  /* USART0 → Alt.2 */
    PERCFG &= ~0x01;
    
    /* P0_3 (MOSI) and P0_5 (CLK) as peripheral function */
    P0SEL  |= (1<<3) | (1<<5);

    /* P0_4 (CS) stays as GPIO — set direction + default high */
    P0SEL  &= ~(1<<4);               /* P0_4 = GPIO, not peripheral */
    P0DIR  |=  (1<<4);               /* P0_4 = output */
    P0_4    =  1;                    /* CS idle high */

    /* P0_3, P0_5 as output */
    P0DIR  |= (1<<3) | (1<<5);

    /* DC pin — P0_2, GPIO output */
    P0SEL &= ~(1<<2);   /* GPIO, not peripheral */
    P0DIR |=  (1<<2);   /* output */
    P0_2   =   1;       /* default high */

    P2SEL &= ~(1<<7);  // PRI2P1
    P2SEL &= ~(1<<6);  // PRI2P0

    /* RST pin — P1_1, GPIO output */
    P1SEL &= ~(1<<1);   /* GPIO, not peripheral */
    P1DIR |=  (1<<1);   /* output */
    P1_1   =   1;       /* default high */

    /* --- USART0 SPI mode ---
     * U0CSR:
     *   bit7 = MODE   0=SPI, 1=UART  → 0
     *   bit5 = SLAVE  0=master       → 0
     * Write 0x00 = SPI master
     */
    U0CSR = 0x00;

    /* --- U0GCR: clock phase, polarity, bit order, baud exponent ---
     *   bit7    = CPOL   clock polarity  0=low when idle  (CPOL=0)
     *   bit6    = CPHA   clock phase     0=sample 1st edge (CPHA=0)
     *   bit5    = ORDER  bit order       1=MSB first
     *   bit[4:0]= BAUD_E baud exponent
     *
     * CPOL=0, CPHA=0 → SPI Mode 0 (matches STM32 POLARITY_LOW + PHASE_1EDGE)
     * ORDER=1 → MSB first
     * BAUD_E=12 → ~500kHz safe for bringup, change to 13 for ~1MHz
     */
    U0GCR = (0<<7) |   /* CPOL=0  */
            (0<<6) |   /* CPHA=0  */
            (1<<5) |   /* MSB first */
            16;        // baud_e = 16, try 2mhz spi clock

    /* BAUD_M=0 (mantissa), combined with BAUD_E gives final rate */
    U0BAUD = 0;

    /* Flush any garbage in the buffer */
    U0UCR |= 0x80;
}

/* -----------------------------------------------------------------------
 * Write one byte — blocks until TX complete
 *
 * Equivalent to: HAL_SPI_Transmit(&hspi1, &value, 1, 1000)
 *
 * TX_BYTE flag is bit1 of U0CSR — set when byte has been shifted out.
 * Must clear it manually before next transfer.
 * ----------------------------------------------------------------------- */
// void DEV_SPI_WriteByte(uint8_t value)
// {
//     U0CSR &= ~0x02;          /* clear TX_BYTE flag before write */
//     U0DBUF  = value;         /* load byte → starts transmission */
//     while (!(U0CSR & 0x02)); /* wait until TX_BYTE set (byte shifted out) */
// }

void DEV_SPI_WriteByte(uint8_t value)
{
    U0CSR &= ~0x02;          // clear TX flag
    U0DBUF = value;          // start transfer

    while (!(U0CSR & 0x02)); // wait TX done
    //while (!(U0CSR & 0x01)); // wait RX done

    (void)U0DBUF;            // dummy read to clear RX
}