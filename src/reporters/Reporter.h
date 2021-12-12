#ifndef Reporter_h
#define Reporter_h

#include "sensors/AirSensor.h"
#include "sensors/EnvironmentSensor.h"
#include "sensors/CO2Sensor.h"

class Reporter
{
public:
  virtual bool report(AirData *air, CalculatedAQI *aqi, EnvironmentData *env, CO2Data *co2) = 0;
  virtual String getLastErrorMessage() = 0;
};

#endif
