#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdarg.h>

void uart_init(void);
void uart_putc(char c);
void uart_puts(__code const char *s);
void uart_printf(__code const char *fmt, ...);

#endif