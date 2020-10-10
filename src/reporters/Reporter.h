#ifndef Reporter_h
#define Reporter_h

#include "sensors/AirSensor.h"

class Reporter
{
public:
    virtual bool report(AirData *air, CalculatedAQI *aqi) = 0;
};

#endif
