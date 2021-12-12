#include "BME680_EnvironmentSensor.h"

BME680_EnvironmentSensor::BME680_EnvironmentSensor(uint16_t gasTemp, uint16_t gasMillis)
    : _sensor(BME680_Class()),
      _gasTemp(gasTemp),
      _gasMillis(gasMillis)
{
  _sensor.begin(I2C_STANDARD_MODE);
  _sensor.setOversampling(TemperatureSensor, Oversample16); // Use enumerated type values
  _sensor.setOversampling(HumiditySensor, Oversample16);    // Use enumerated type values
  _sensor.setOversampling(PressureSensor, Oversample16);    // Use enumerated type values
  _sensor.setIIRFilter(IIR4);                               // Use enumerated type values
  _sensor.setGas(_gasTemp, _gasMillis);                     // 320°c for 150 milliseconds
}

bool BME680_EnvironmentSensor::getEnvironmentData(EnvironmentData *data)
{
  static int32_t temp, hum, pressure, gas;
  bool success = _sensor.getSensorData(temp, hum, pressure, gas);
  if (!success)
  {
    return false;
  }

  data->tempC = (float)temp / 100 + BME680_TEMPERATURE_OFFSET;
  data->humPct = (float)hum / 1000;
  data->pressure = (float)pressure / 100;
  data->gasResistance = (float)gas / 100;
  data->iaq = calculateIAQ(data->gasResistance, data->humPct);

  return true;
}

void BME680_EnvironmentSensor::loop()
{
}
