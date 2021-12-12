#include <Arduino.h>

#include "EnvironmentSensor.h"

float calculateIAQ(float gasResistance, float humPct)
{
  return log(gasResistance) + 0.04 * humPct;
}
