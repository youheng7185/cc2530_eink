#ifndef UART_RX_H
#define UART_RX_H

#include <stdint.h>
#include <cc2530.h>

#define FRAMEBUFFER_SIZE 2756

void uart_process_command(void);
extern __xdata uint8_t framebuffer[FRAMEBUFFER_SIZE];

#endif