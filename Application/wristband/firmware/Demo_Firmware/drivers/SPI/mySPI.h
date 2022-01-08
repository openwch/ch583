#ifndef __MYSPI_H
#define __MYSPI_H
#include <stdint.h>
#include <stdbool.h>

#define BUFFER_LENGTH   80

struct SPI_t{
    uint8_t txBuffer[BUFFER_LENGTH];
    uint8_t txBufferIndex;
    uint8_t txBufferLength;
    bool transmitting;
};

void MySPIinit(void);
void MySPIdisbale(void);
void MySPIsendbuf( uint8_t *pbuf, uint16_t len);
void MySPIsenddata(uint8_t data);

#endif
