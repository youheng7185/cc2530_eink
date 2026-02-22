#ifndef HAL_DELAY_H
#define HAL_DELAY_H

#include <stdint.h>
#include "wdt.h"   /* for WDT_FEED() */

/*
 * Blocking delay @ 32MHz.
 * Max 500ms — above that you risk the watchdog even with feeding,
 * so call multiple times if you need longer.
 *
 * Inner loop is ~535 iterations ≈ 1ms @ 32MHz.
 */
static void HAL_Delay(uint16_t ms)
{
    uint32_t i, j;
    ms = ms << 1;
    // if (ms > 500) ms = 500;          /* clamp to safe maximum */
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 535; j++);
        WDT_FEED();
    }
}

#endif /* HAL_DELAY_H */