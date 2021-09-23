#ifndef BME680_EnvironmentSensor_h
#define BME680_EnvironmentSensor_h

#include <Zanshin_BME680.h>

#include "EnvironmentSensor.h"

class BME680_EnvironmentSensor : public EnvironmentSensor
{
public:
    BME680_EnvironmentSensor(uint16_t gasTemp, uint16_t gasMillis);
    bool getEnvironmentData(EnvironmentData *data);
    void loop();

private:
    BME680_Class _sensor;
    uint16_t _gasTemp;
    uint16_t _gasMillis;
};

#endif
