#include "MHZ19B_CO2Sensor.h"

MHZ19B_CO2Sensor::MHZ19B_CO2Sensor(Stream *serial, bool autoCalibration) : _sensor(ErriezMHZ19B(serial))
{
  _sensor.setAutoCalibration(autoCalibration);
}
MHZ19B_CO2Sensor::~MHZ19B_CO2Sensor() {}

void MHZ19B_CO2Sensor::loop()
{
  if (_ready)
    return;

  if (_sensor.isReady())
  {
    int16_t ppm = _sensor.readCO2();
    if (ppm < 0)
    {
      switch (ppm)
      {
      case MHZ19B_RESULT_ERR_CRC:
        _lastError = String("CRC error");
        break;
      case MHZ19B_RESULT_ERR_TIMEOUT:
        _lastError = String("RX timeout");
        break;
      default:
        _lastError = String("Unknown error: ") + String(ppm);
        break;
      }
      return;
    }

    _data.ppm = ppm;
    _data.timestamp = millis();
    _ready = true;
  }
}

bool MHZ19B_CO2Sensor::calibrate()
{
  return _sensor.startZeroCalibration() == MHZ19B_RESULT_OK;
}

bool MHZ19B_CO2Sensor::getCO2Data(CO2Data *data)
{
  if (!_ready)
  {
    return false;
  }

  memcpy(data, &_data, sizeof(CO2Data));
  _ready = false;

  return true;
}
