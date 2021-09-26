#include <EEPROM.h>

#ifdef ESP8266
#include <SoftwareSerial.h>
#elif defined(ESP32)
#include <HardwareSerial.h>
#endif

#include "util.h"

int util::rnd(float val)
{
    return (int)round(val);
}

void util::clearEEPROM()
{
    int len = EEPROM.length();

    for (int i = 0; i < len; i++)
    {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}

#ifdef ESP8266
Stream *util::getSerial(uint8_t rxPin, uint8_t txPin)
{
    SoftwareSerial *serial = new SoftwareSerial(rxPin, txPin);
    serial->begin(9600);
    return serial;
}
#elif defined(ESP32)
Stream *util::getSerial(uint8_t uartNo,
                        unsigned long baud,
                        uint32_t config,
                        int8_t rxPin,
                        int8_t txPin)
{
    HardwareSerial *serial = new HardwareSerial(uartNo);
    serial->begin(baud, config, rxPin, txPin);

    return serial;
}
#endif

int util::sortUint16Desc(const void *cmp1, const void *cmp2)
{
    uint16_t a = *((uint16_t *)cmp1);
    uint16_t b = *((uint16_t *)cmp2);
    return b - a;
}

uint16_t util::medianValue(uint16_t *values, int count)
{
    qsort(values, count, sizeof(values[0]), sortUint16Desc);

    return values[(count / 2) - 1];
}
