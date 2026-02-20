#ifndef EPD_BUSY_H
#define EPD_BUSY_H

#include <cc2530.h>
#include <stdint.h>

/*
 * EINK_BUSY → P1_2
 * UC8151/IL0373: HIGH = busy, LOW = ready
 */

static void EPD_Busy_Init(void)
{
    P1SEL &= ~(1<<2);   /* GPIO, not peripheral */
    P1DIR &= ~(1<<2);   /* input */
}

/* returns 1 if busy, 0 if ready */
static uint8_t EPD_Busy(void)
{
    return P1_2 ? 1 : 0;
}

#endif /* EPD_BUSY_H */