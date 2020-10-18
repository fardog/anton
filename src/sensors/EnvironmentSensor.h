#ifndef EnvironmentSensor_h
#define EnvironmentSensor_h

struct EnvironmentData
{
    float tempC;
    float humPct;
    float pressure;
    float gasResistance;
};

class EnvironmentSensor
{
public:
    virtual bool getEnvironmentData(EnvironmentData *data) = 0;
};

#endif
