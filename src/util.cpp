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
int8_t util::stringToPin(char *s)
{
    if (strcmp(s, "D1") == 0)
    {
        return D1;
    }
    if (strcmp(s, "D2") == 0)
    {
        return D2;
    }
    if (strcmp(s, "D3") == 0)
    {
        return D3;
    }
    if (strcmp(s, "D4") == 0)
    {
        return D4;
    }
    if (strcmp(s, "D5") == 0)
    {
        return D5;
    }
    if (strcmp(s, "D6") == 0)
    {
        return D6;
    }
    if (strcmp(s, "D7") == 0)
    {
        return D7;
    }
    if (strcmp(s, "D8") == 0)
    {
        return D8;
    }
    if (strcmp(s, "D9") == 0)
    {
        return D9;
    }
    if (strcmp(s, "D10") == 0)
    {
        return D10;
    }

    return -1;
}

Stream *util::getSerial(char *rx, char *tx)
{
    int8_t rxPin = stringToPin(rx);
    int8_t txPin = stringToPin(tx);

    SoftwareSerial *serial = new SoftwareSerial(rxPin, txPin);
    serial->begin(9600);
    return serial;
}
#elif defined(ESP32)
int8_t util::stringToUart(char *s)
{
    if (strcmp(s, "U0") == 0)
    {
        return 0;
    }
    if (strcmp(s, "U1") == 0)
    {
        return 1;
    }
    if (strcmp(s, "U2") == 0)
    {
        return 2;
    }

    return -1;
}

Stream *util::getSerial(char *uart)
{
    int8_t uartNo = stringToUart(uart);

    HardwareSerial *serial = new HardwareSerial(uartNo);
    serial->begin(9600);

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
