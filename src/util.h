#ifndef util_h
#define util_h

#include <Arduino.h>

namespace util
{
    int rnd(float val);
    void clearEEPROM();
    int8_t stringToPin(char *s);
    int sortUint16Desc(const void *cmp1, const void *cmp2);
    uint16_t medianValue(uint16_t *values, int count);
} // namespace util

#endif
