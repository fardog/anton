#ifndef MHZ19B_CO2Sensor_h
#define MHZ19B_CO2Sensor_h

#include <ErriezMHZ19B.h>

#include "CO2Sensor.h"

class MHZ19B_CO2Sensor : public CO2Sensor
{
public:
  MHZ19B_CO2Sensor(Stream *serial, bool autoCalibration = false);
  ~MHZ19B_CO2Sensor();

  bool getCO2Data(CO2Data *data);
  void loop();
  bool calibrate();
  String getLastError() { return _lastError; };

private:
  ErriezMHZ19B _sensor;
  CO2Data _data{};
  bool _ready{false};
  String _lastError{""};
};

#endif
