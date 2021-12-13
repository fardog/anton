#include "BME680_EnvironmentSensor.h"

BME680_EnvironmentSensor::BME680_EnvironmentSensor(TwoWire &i2c)
    : _sensor(Bsec())
{
  _sensor.begin(BME680_I2C_ADDR_SECONDARY, i2c);
  _sensor.updateSubscription(_sensorList, 11, BSEC_SAMPLE_RATE_LP);
}

bool BME680_EnvironmentSensor::getEnvironmentData(EnvironmentData *data)
{
  if (!_ready)
  {
    return false;
  }

  _ready = false;

  memcpy(data, &_data, sizeof(EnvironmentData));

  return true;
}

void BME680_EnvironmentSensor::loop()
{
  if (_sensor.run())
  {
    _ready = true;
    _data.tempC = _sensor.temperature;
    _data.humPct = _sensor.humidity;
    _data.pressure = _sensor.pressure;
    _data.gasResistance = _sensor.gasResistance;
    _data.iaq = _sensor.staticIaq;
    _data.iaqAccuracy = _sensor.iaqAccuracy;
    _data.co2Ppm = _sensor.co2Equivalent;
    _data.co2Accuracy = _sensor.co2Accuracy;
    _data.breathVoc = _sensor.breathVocEquivalent;
    _data.breathVocAccuracy = _sensor.breathVocAccuracy;
  }
  else
  {
    if (_sensor.status != BSEC_OK)
    {
      if (_sensor.status < BSEC_OK)
      {
        _lastError = "BSEC error code : " + String(_sensor.status);
      }
      else
      {
        _lastError = "BSEC warning code : " + String(_sensor.status);
      }
    }

    if (_sensor.bme680Status != BME680_OK)
    {
      if (_sensor.bme680Status < BME680_OK)
      {
        _lastError = "BME680 error code : " + String(_sensor.bme680Status);
      }
      else
      {
        _lastError = "BME680 warning code : " + String(_sensor.bme680Status);
      }
    }
  }
}
