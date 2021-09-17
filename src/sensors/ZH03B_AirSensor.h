#ifndef ZH03b_AirSensor_h
#define ZH03b_AirSensor_h

#include <SD_ZH03B.h>

#include "AirSensor.h"

class ZH03B_AirSensor : public AirSensor
{
public:
    ZH03B_AirSensor(Stream &serial, bool debug = false);
    ~ZH03B_AirSensor();

    bool getAirData(AirData *data);
    void loop();
    bool sleep();
    bool wake();

private:
    SD_ZH03B _sensor;
    bool _debug;
    bool _read_sensor_data(uint16_t *arr);
};

#endif
