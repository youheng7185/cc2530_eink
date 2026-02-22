#ifndef GXGDEW0213Z16_H
#define GXGDEW0213Z16_H

#include <stdint.h>

void EPD_Init(void);
void EPD_Clear(void);
void EPD_Test(void);
void EPD_SendFrame(uint8_t *framebuffer);

#endif