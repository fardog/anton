#ifndef AirSensor_h
#define AirSensor_h

#include <AQI.h>

#include "interfaces/Looper.h"

struct AirData
{
  uint16_t p1_0;
  uint16_t p2_5;
  uint16_t p10_0;
  unsigned int timestamp;
};

struct CalculatedAQI
{
  float value;
  char pollutant[6];
};

bool calculateAQI(AirData sample, CalculatedAQI *aqi);

class AirSensor : public Looper
{
public:
  virtual bool getAirData(AirData *data) = 0;
  virtual bool sleep() = 0;
  virtual bool wake() = 0;
};

#endif
