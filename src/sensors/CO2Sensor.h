#ifndef CO2Sensor_h
#define CO2Sensor_h

#include <Arduino.h>

#include "interfaces/Looper.h"

struct CO2Data
{
  uint16_t ppm;
  unsigned int timestamp;
};

class CO2Sensor : public Looper
{
public:
  virtual bool getCO2Data(CO2Data *data) = 0;
  virtual bool calibrate() = 0;
  virtual String getLastError() = 0;
};

#endif
