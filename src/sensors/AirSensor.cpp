#include <Arduino.h>
#include <AQI.h>

#include "AirSensor.h"

bool calculateAQI(AirData sample, CalculatedAQI *aqi)
{
  AQI::Measurement list[2] = {
      AQI::Measurement(AQI::PM2_5, sample.p2_5),
      AQI::Measurement(AQI::PM10, sample.p10_0)};

  AQI::Measurements measurements = AQI::Measurements(list, 2);

  aqi->value = measurements.getAQI();
  if (aqi->value == -1)
  {
    return false;
  }

  AQI::Pollutant pollutant = measurements.getPollutant();
  switch (pollutant)
  {
  case AQI::PM2_5:
    strcpy(aqi->pollutant, "p2_5");
    break;
  case AQI::PM10:
    strcpy(aqi->pollutant, "p10_0");
    break;
  default:
    return false;
  }

  return true;
}
