/* -----------------------------------------------------------------------
 * Watchdog
 *
 * WDCTL bits (corrected):
 *   [7:4] = CLR   feed sequence: write 0xA0 then 0x50 to this field
 *   [3]   = EN    set-only, cannot clear once set
 *   [2]   = MODE  0=watchdog, 1=timer
 *   [1:0] = INT   interval: 00=max(~1s), 01=long, 10=medium, 11=short
 *
 * Feed sequence writes to HIGH nibble, preserving LOW nibble (EN/MODE/INT).
 * ----------------------------------------------------------------------- */
#include <cc2530.h>
#include <stdint.h>
#include <stdarg.h>
#include "wdt.h"

void wdt_init(void)
{
    /* Set maximum interval, then feed immediately.
     * If previous firmware left watchdog running, this resets
     * the counter and gives us a full ~1s window. */
    WDCTL = (WDCTL & 0xF0) | WDTIMX;
    WDT_FEED();
}
