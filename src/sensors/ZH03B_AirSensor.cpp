#include "ZH03B_AirSensor.h"

ZH03B_AirSensor::ZH03B_AirSensor(Stream &serial, bool debug) : _sensor(SD_ZH03B(serial)), _debug(debug)
{
    _sensor.setMode(SD_ZH03B::IU_MODE);
}

ZH03B_AirSensor::~ZH03B_AirSensor() {}

bool ZH03B_AirSensor::getAirData(AirData *data)
{
    if (_sensor.readData())
    {
        data->p1_0 = _sensor.getPM1_0();
        data->p2_5 = _sensor.getPM2_5();
        data->p10_0 = _sensor.getPM10_0();

        return true;
    }
    else
    {
        return false;
    }
}

void ZH03B_AirSensor::loop() {}

bool ZH03B_AirSensor::sleep()
{
    return _sensor.sleep();
}

bool ZH03B_AirSensor::wake()
{
    return _sensor.wakeup();
}
