#ifndef BME680_EnvironmentSensor_h
#define BME680_EnvironmentSensor_h

#include <Zanshin_BME680.h>

#include "EnvironmentSensor.h"

class BME680_EnvironmentSensor : public EnvironmentSensor
{
public:
    BME680_EnvironmentSensor();
    bool getEnvironmentData(EnvironmentData *data);

private:
    BME680_Class _sensor;
};

#endif
