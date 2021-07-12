#include "PMS_AirSensor.h"

PMS_AirSensor::PMS_AirSensor(Stream &serial, bool debug) : _sensor(PMS(serial)), _debug(debug) {}
PMS_AirSensor::~PMS_AirSensor() {}

bool PMS_AirSensor::getAirData(AirData *data)
{
	bool success = _sensor.readUntil(_buf);
	if (!success)
	{
		return false;
	}

	data->p1_0 = _buf.PM_AE_UG_1_0;
	data->p2_5 = _buf.PM_AE_UG_2_5;
	data->p10_0 = _buf.PM_AE_UG_10_0;

	return true;
}

bool PMS_AirSensor::wake()
{
	_sensor.wakeUp();
	return true;
}

bool PMS_AirSensor::sleep()
{
	_sensor.sleep();
	return true;
}
