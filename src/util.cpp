#include <EEPROM.h>

#include "util.h"

int util::rnd(float val)
{
    return (int)round(val);
}

void util::clearEEPROM()
{
    int len = EEPROM.length();

    for (int i = 0; i < 4096; i++)
    {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}
