#ifndef util_h
#define util_h

#include <Arduino.h>

namespace util
{
    int rnd(float val);
    void clearEEPROM();
    int sortUint16Desc(const void *cmp1, const void *cmp2);
    uint16_t medianValue(uint16_t *values, int count);

#ifdef ESP8266
    Stream *getSerial(uint8_t txPin, uint8_t rxPin);
#elif defined(ESP32)
    Stream *getSerial(uint8_t uartNo);
#endif
} // namespace util

#endif
