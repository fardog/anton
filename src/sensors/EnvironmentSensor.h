#ifndef EnvironmentSensor_h
#define EnvironmentSensor_h

struct EnvironmentData
{
  float tempC;
  float tempCRaw;
  float humPct;
  float humPctRaw;
  float pressure;
  float gasResistance;
  float iaq;
  uint8_t iaqAccuracy;
  float co2Ppm;
  uint8_t co2Accuracy;
  float breathVoc;
  uint8_t breathVocAccuracy;
};

class EnvironmentSensor
{
public:
  virtual bool getEnvironmentData(EnvironmentData *data) = 0;
  virtual void loop() = 0;
  virtual String getLastError() = 0;
};

#endif
