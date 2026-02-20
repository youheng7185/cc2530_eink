#ifndef WDT_H
#define WDT_H

#include <cc2530.h>

#define WDCLP1  0xA0
#define WDCLP2  0x50
#define WDTIMX  0x00   /* maximum interval ~1s @ 32kHz */

#define WDT_FEED()  do { WDCTL = (WDCTL & 0x0F) | WDCLP1; \
                         WDCTL = (WDCTL & 0x0F) | WDCLP2; } while(0)

void wdt_init(void);

#endif