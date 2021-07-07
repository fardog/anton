#ifndef PMS_AirSensor_h
#define PMS_AirSensor_h

#include <PMS.h>

#include "AirSensor.h"

class PMS_AirSensor : public AirSensor
{
public:
	PMS_AirSensor(Stream &serial, bool debug = false);
	~PMS_AirSensor();

	bool getAirData(AirData *data);
	bool wake();
	bool sleep();

private:
	PMS _sensor;
	PMS::DATA _buf;
	bool _debug;
};

#endif
