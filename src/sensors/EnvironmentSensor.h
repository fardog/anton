#ifndef EnvironmentSensor_h
#define EnvironmentSensor_h

struct EnvironmentData
{
  float tempC;
  float humPct;
  float pressure;
  float gasResistance;
  float iaq;
};

float calculateIAQ(float gasResistance, float humPct);

class EnvironmentSensor
{
public:
  virtual bool getEnvironmentData(EnvironmentData *data) = 0;
  virtual void loop() = 0;
};

#endif
