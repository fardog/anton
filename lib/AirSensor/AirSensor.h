#ifndef AirSensor_h
#define AirSensor_h

struct AirData
{
    int p1_0;
    int p2_5;
    int p10_0;
};

class AirSensor
{
public:
    virtual bool getAirData(AirData *data) = 0;
    virtual bool wake() = 0;
    virtual bool sleep() = 0;
};

#endif
