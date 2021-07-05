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
    int8_t stringToPin(char *s);
    Stream *getSerial(char *rx, char *tx);
#elif defined(ESP32)
    int8_t stringToUart(char *s);
    Stream *getSerial(char *uart);
#endif
} // namespace util

#endif
