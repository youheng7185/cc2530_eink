#ifndef SPI_H
#define SPI_H

#include <stdint.h>

void SPI_Init(void);
void DEV_SPI_WriteByte(uint8_t value);
void SPI_CS_LOW(void);
void SPI_CS_HIGH(void);
void SPI_DC_LOW(void);
void SPI_DC_HIGH(void);
void SPI_RST_LOW(void);
void SPI_RST_HIGH(void);

#endif /* SPI_H */