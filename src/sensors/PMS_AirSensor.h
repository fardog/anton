#ifndef PMS_AirSensor_h
#define PMS_AirSensor_h

#include <PMS.h>

#include "AirSensor.h"

#define PMS_NUM_SAMPLES 10

class PMS_AirSensor : public AirSensor
{
public:
	PMS_AirSensor(Stream &serial, bool debug = false);
	~PMS_AirSensor();

	bool getAirData(AirData *data);
	void loop();
	bool sleep();
	bool wake();

private:
	PMS _sensor;
	bool _debug;

	uint16_t p1_0[PMS_NUM_SAMPLES];
	uint16_t p2_5[PMS_NUM_SAMPLES];
	uint16_t p10_0[PMS_NUM_SAMPLES];

	PMS::DATA _buf;
	int _cur = 0;
	bool _ready = false;
};

#endif
