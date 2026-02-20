#include <cc2530.h>
#include <stdint.h>
#include <stdarg.h>

/* -----------------------------------------------------------------------
 * UART — USART1 Alt.2 (P1_6=TX, P1_7=RX)
 * 115200 baud @ 32 MHz
 *
 * Baud rate formula:
 *   baud = (256 + U1BAUD) * 2^(U1GCR.BAUD_E) / 2^28 * F_cpu
 *
 * For 115200 @ 32MHz: BAUD_E=11, BAUD_M=216  → 115273 baud (0.06% error)
 * ----------------------------------------------------------------------- */
void uart_init(void)
{
    /* Route USART1 to Alt.2: P1_4=CT, P1_5=RT, P1_6=TX, P1_7=RX */
    PERCFG |= 0x02;              /* USART1 Alt.2 */

    /* P1_6 as peripheral (TX output), P1_7 as peripheral (RX input) */
    P1SEL  |= (1<<6) | (1<<7);
    P1DIR  |= (1<<6);            /* TX = output */
    P1DIR  &= ~(1<<7);           /* RX = input  */

    /* USART1 in UART mode */
    U1CSR  = 0x80;               /* UART mode (bit7=1), receiver disabled for now */
    U1UCR  = 0x02;               /* 8N1, no flow control, high stop bit */

    /* Baud rate: 115200 @ 32 MHz */
    U1BAUD = 216;                /* BAUD_M */
    U1GCR  = 11;                 /* BAUD_E (bits[4:0]), MSB first = 0 (LSB first for UART) */

    /* Enable receiver */
    U1CSR |= 0x40;
}

/* Send one byte, blocking */
void uart_putc(char c)
{
    /* CR+LF on newline (handy for terminals) */
    if (c == '\n')
        uart_putc('\r');

    U1DBUF = c;
    while (!(U1CSR & 0x02));    /* wait TX_BYTE flag (bit1) */
    U1CSR &= ~0x02;             /* clear flag */
}

/* Send null-terminated string */
void uart_puts(__code const char *s)
{
    while (*s)
        uart_putc(*s++);
}

/* -----------------------------------------------------------------------
 * Minimal printf — supports:
 *   %c  character
 *   %s  string (__code const char* or __xdata char*)
 *   %d  signed 16-bit decimal
 *   %u  unsigned 16-bit decimal
 *   %x  unsigned 16-bit hex (lowercase, no leading zeros)
 *   %X  unsigned 16-bit hex (uppercase, no leading zeros)
 *   %02x / %04x  zero-padded hex
 *   %%  literal %
 *
 * No malloc, no float, fits in ~400 bytes of code.
 * ----------------------------------------------------------------------- */

/* Reverse a string in-place (helper for int→string conversion) */
static void strrev_local(__xdata char *s, uint8_t len)
{
    uint8_t i = 0, j = len - 1;
    char tmp;
    while (i < j) {
        tmp  = s[i];
        s[i] = s[j];
        s[j] = tmp;
        i++; j--;
    }
}

/* Write unsigned int to buffer, return length */
static uint8_t uint_to_str(uint16_t val, __xdata char *buf)
{
    uint8_t len = 0;
    if (val == 0) { buf[len++] = '0'; }
    while (val) {
        buf[len++] = '0' + (val % 10);
        val /= 10;
    }
    strrev_local(buf, len);
    buf[len] = '\0';
    return len;
}

static uint8_t uint_to_hex(uint16_t val, __xdata char *buf, uint8_t upper)
{
    __code static const char lc[] = "0123456789abcdef";
    __code static const char uc[] = "0123456789ABCDEF";
    __code const char *digits = upper ? uc : lc;
    uint8_t len = 0;
    if (val == 0) { buf[len++] = '0'; }
    while (val) {
        buf[len++] = digits[val & 0xF];
        val >>= 4;
    }
    strrev_local(buf, len);
    buf[len] = '\0';
    return len;
}

void uart_printf(__code const char *fmt, ...)
{
    va_list ap;
    static __xdata char numbuf[8];   /* static: 8051 funcs have no xdata stack */
    char c;
    uint8_t pad, len, i;

    va_start(ap, fmt);

    while ((c = *fmt++) != '\0')
    {
        if (c != '%') {
            uart_putc(c);
            continue;
        }

        /* parse optional zero-pad width (e.g. %02x) */
        pad = 0;
        c = *fmt++;
        if (c == '0') {
            c = *fmt++;
            pad = c - '0';   /* single digit width only */
            c = *fmt++;
        }

        switch (c)
        {
        case 'c':
            uart_putc((char)va_arg(ap, int));
            break;

        case 's':
            /* accepts __code string literals */
            uart_puts(va_arg(ap, __code const char *));
            break;

        case 'd': {
            int16_t v = (int16_t)va_arg(ap, int);
            if (v < 0) { uart_putc('-'); v = -v; }
            len = uint_to_str((uint16_t)v, numbuf);
            for (i = len; i < pad; i++) uart_putc('0');
            for (i = 0; i < len; i++) uart_putc(numbuf[i]);
            break;
        }

        case 'u': {
            uint16_t v = (uint16_t)va_arg(ap, unsigned int);
            len = uint_to_str(v, numbuf);
            for (i = len; i < pad; i++) uart_putc('0');
            for (i = 0; i < len; i++) uart_putc(numbuf[i]);
            break;
        }

        case 'x':
        case 'X': {
            uint16_t v = (uint16_t)va_arg(ap, unsigned int);
            len = uint_to_hex(v, numbuf, (c == 'X'));
            for (i = len; i < pad; i++) uart_putc('0');
            for (i = 0; i < len; i++) uart_putc(numbuf[i]);
            break;
        }

        case '%':
            uart_putc('%');
            break;

        default:
            uart_putc('%');
            uart_putc(c);
            break;
        }
    }

    va_end(ap);
}