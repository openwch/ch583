#ifndef __MYI2C_H
#define __MYI2C_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus

#define BUFFER_LENGTH 32

#define TW_MT_SLA_NACK  1
#define TW_MT_DATA_NACK 2


class TwoWire {
private:
    static uint8_t rxBuffer[];
    static uint8_t rxBufferIndex;
    static uint8_t rxBufferLength;

    static uint8_t txAddress;
    static uint8_t txBuffer[];
    static uint8_t txBufferIndex;
    static uint8_t txBufferLength;

    static uint8_t transmitting;
    static void (*user_onRequest)(void);
    static void (*user_onReceive)(int);
    static void onRequestService(void);
    static void onReceiveService(uint8_t*, int);

    static bool isInit;

public:
    void begin();
    void end();
    void beginTransmission(uint8_t);
    uint8_t endTransmission(bool);
    uint8_t endTransmission(void);
    uint8_t requestFrom(uint8_t, uint8_t);
    uint8_t requestFrom(uint8_t, uint8_t, bool);
    uint8_t requestFrom(uint8_t, uint8_t, uint32_t, uint8_t, uint8_t);
    uint8_t requestFrom(int, int, int);
    size_t write(const uint8_t);
    size_t write(const uint8_t *, size_t);
    int available(void);
    int read(void);
    int peek(void);
    void flush(void);
};

void scanI2Cdevice(void);

extern TwoWire Wire;

#endif

#endif
















