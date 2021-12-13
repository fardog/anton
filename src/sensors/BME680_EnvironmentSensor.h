#ifndef BME680_EnvironmentSensor_h
#define BME680_EnvironmentSensor_h

#include <bsec.h>

#include "EnvironmentSensor.h"

class BME680_EnvironmentSensor : public EnvironmentSensor
{
public:
  BME680_EnvironmentSensor(TwoWire &i2c);
  bool getEnvironmentData(EnvironmentData *data);
  void loop();
  String getLastError() { return _lastError; };

private:
  Bsec _sensor;
  bool _ready{false};
  String _lastError{""};

  bsec_virtual_sensor_t _sensorList[11] = {
      BSEC_OUTPUT_RAW_TEMPERATURE,
      BSEC_OUTPUT_RAW_PRESSURE,
      BSEC_OUTPUT_RAW_HUMIDITY,
      BSEC_OUTPUT_RAW_GAS,
      BSEC_OUTPUT_IAQ,
      BSEC_OUTPUT_STATIC_IAQ,
      BSEC_OUTPUT_CO2_EQUIVALENT,
      BSEC_OUTPUT_COMPENSATED_GAS,
      BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  EnvironmentData _data{};
};

#endif
