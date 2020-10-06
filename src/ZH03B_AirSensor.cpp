#include "ZH03B_AirSensor.h"

ZH03B_AirSensor::ZH03B_AirSensor(Stream &serial, bool debug) : _sensor(SD_ZH03B(serial)), _debug(debug)
{
    _sensor.setMode(SD_ZH03B::IU_MODE);
    sleep();
}

ZH03B_AirSensor::~ZH03B_AirSensor() {}

bool ZH03B_AirSensor::getAirData(AirData *data)
{
    uint16_t values[3];
    if (!_read_sensor_data(values))
    {
        return false;
    }

    data->p1_0 = (int)values[0];
    data->p2_5 = (int)values[1];
    data->p10_0 = (int)values[2];

    return true;
}

bool ZH03B_AirSensor::wake()
{
    return _sensor.wakeup();
}

bool ZH03B_AirSensor::sleep()
{
    return _sensor.sleep();
}

bool ZH03B_AirSensor::_read_sensor_data(uint16_t *arr)
{
    Serial.println("sensor: sampling");
    if (_sensor.readData())
    {
        arr[0] = _sensor.getPM1_0();
        arr[1] = _sensor.getPM2_5();
        arr[2] = _sensor.getPM10_0();

        return true;
    }
    else
    {
        Serial.println("sensor: Error reading stream or Check Sum Error");
        return false;
    }
}
