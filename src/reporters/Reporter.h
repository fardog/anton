#ifndef Reporter_h
#define Reporter_h

#include "sensors/AirSensor.h"
#include "sensors/EnvironmentSensor.h"

class Reporter
{
public:
    virtual bool report(AirData *air, CalculatedAQI *aqi, EnvironmentData *env) = 0;
};

#endif
