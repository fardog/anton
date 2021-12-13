#ifndef util_h
#define util_h

#include <Arduino.h>

namespace util
{
  int rnd(float val);
  float rnd(float val, uint places);
  void clearEEPROM();
  int sortUint16Desc(const void *cmp1, const void *cmp2);
  uint16_t medianValue(uint16_t *values, int count);

#ifdef ESP8266
  Stream *getSerial(uint8_t txPin, uint8_t rxPin);
#elif defined(ESP32)
  Stream *getSerial(uint8_t uartNo,
                    unsigned long baud = 9600,
                    uint32_t config = SERIAL_8N1,
                    int8_t rxPin = -1,
                    int8_t txPin = -1);
#endif
} // namespace util

#endif
