#include "PMS_AirSensor.h"
#include "util.h"

PMS_AirSensor::PMS_AirSensor(Stream &serial, bool debug) : _sensor(PMS(serial)), _debug(debug) {}
PMS_AirSensor::~PMS_AirSensor() {}

bool PMS_AirSensor::getAirData(AirData *data)
{
	if (!_ready)
	{
		return false;
	}

	data->p1_0 = util::medianValue(p1_0, PMS_NUM_SAMPLES);
	data->p2_5 = util::medianValue(p2_5, PMS_NUM_SAMPLES);
	data->p10_0 = util::medianValue(p10_0, PMS_NUM_SAMPLES);

	_ready = false;

	return true;
}

void PMS_AirSensor::loop()
{
	// we have enough data and are ready to be read
	if (_ready)
	{
		return;
	}

	bool success = _sensor.read(_buf);
	if (success)
	{
		p1_0[_cur] = _buf.PM_AE_UG_1_0;
		p2_5[_cur] = _buf.PM_AE_UG_2_5;
		p10_0[_cur] = _buf.PM_AE_UG_10_0;

		_cur += 1;
		if (_cur > PMS_NUM_SAMPLES - 1)
		{
			_cur = 0;
			_ready = true;
		}
	}
}

bool PMS_AirSensor::sleep()
{
	_sensor.sleep();
	// no feedback, so just assume it worked
	return true;
}

bool PMS_AirSensor::wake()
{
	_sensor.wakeUp();
	// no feedback, so just assume it worked
	return true;
}
