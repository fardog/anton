#include <Arduino.h>
#include "Anton.h"

Anton::Anton(Reporter *reporter,
             AirSensor *airSensor,
             CO2Sensor *co2Sensor,
             EnvironmentSensor *environmentSensor,
             uint16_t timeBetweenMeasurements)
    : _reporter(reporter),
      _airSensor(airSensor),
      _co2Sensor(co2Sensor),
      _environmentSensor(environmentSensor),
      _states{
          {StateId::STARTUP, 0, 0, StateId::WAKE_SENSORS},
          {StateId::WAKE_SENSORS, 0, 0, StateId::WARM_UP},
          {StateId::WARM_UP, 30000, 0, StateId::SAMPLE_PARTICULATE},
          {StateId::SAMPLE_PARTICULATE, 0, 30000, StateId::SAMPLE_CO2},
          {StateId::SAMPLE_CO2, 0, 30000, StateId::SAMPLE_ENVIRONMENT},
          {StateId::SAMPLE_ENVIRONMENT, 0, 10000, StateId::SLEEP_SENSORS},
          {StateId::SLEEP_SENSORS, 0, 10000, StateId::REPORT},
          {StateId::REPORT, 0, 30000, StateId::SLEEP},
          {StateId::SLEEP, timeBetweenMeasurements, 0, StateId::WAKE_SENSORS},
      }
{
  _state = _states[0];
}

Anton::~Anton() {}

void Anton::loop()
{
  unsigned long now = millis();

  if (_state.timeout > 0 && now > _lastTransition + _state.timeout)
  {
    Serial.printf("state %d timed out; moving to next state\n", _state.state);
    _stateFailed[_state.state] = true;
    _nextState();
    return;
  }

  if (now < _stateStart)
  {
    return;
  }

  switch (_state.state)
  {
  case STARTUP:
    _startup();
    break;
  case WAKE_SENSORS:
    _wakeSensors();
    break;
  case WARM_UP:
    _warmUp();
    break;
  case SAMPLE_PARTICULATE:
    _sampleParticulate();
    break;
  case SAMPLE_CO2:
    _sampleCO2();
    break;
  case SAMPLE_ENVIRONMENT:
    _sampleEnvironment();
  case SLEEP_SENSORS:
    _sleepSensors();
    break;
  case REPORT:
    _report();
    break;
  case SLEEP:
    _sleep();
    break;
  }
}

void Anton::_nextState()
{
  Serial.printf("state change: %d -> %d, delaying %dms\n", _state.state, _state.next, _state.delay);
  _stateStart = millis() + _state.delay;
  _lastTransition = _stateStart;
  _state = _states[_state.next];
  _stateFailed[_state.state] = false;
}

void Anton::_retry(uint16_t delay)
{
  Serial.printf("retrying state %d, delaying %dms\n", _state.state, delay);
  _stateStart = millis() + delay;
}

void Anton::_startup()
{
  _nextState();
}

void Anton::_wakeSensors()
{
  if (!_airSensor)
  {
    _nextState();
    return;
  }

  if (_airSensor->wake())
  {
    _nextState();
  }
  else
  {
    Serial.println("failed to wake air sensor");
  }
}

void Anton::_warmUp()
{
  Serial.println("warming up");
  _nextState();
}

void Anton::_sampleParticulate()
{
  if (_airSensor)
  {
    _airSensor->loop();
    if (_airSensor->getAirData(&_airData))
    {
      Serial.printf("Particulate: PM1.0, PM2.5, PM10=[%d %d %d]\n",
                    _airData.p1_0,
                    _airData.p2_5,
                    _airData.p10_0);

      _nextState();
    }
  }
  else
  {
    _nextState();
  }
}

void Anton::_sampleCO2()
{
  if (_co2Sensor)
  {
    _co2Sensor->loop();
    if (_co2Sensor->getCO2Data(&_co2Data))
    {
      _nextState();
    }
  }
  else
  {
    _nextState();
  }
}

void Anton::_sampleEnvironment()
{
  if (_environmentSensor)
  {
    _environmentSensor->loop();
    if (_environmentSensor->getEnvironmentData(&_environmentData))
    {
      Serial.printf("Environment: %.2f°C, %.2f%%rh, IAQ %.2f, %.2fhPa, %.2fOhm\n",
                    _environmentData.tempC,
                    _environmentData.humPct,
                    _environmentData.iaq,
                    _environmentData.pressure,
                    _environmentData.gasResistance);
      _nextState();
    }
  }
  else
  {

    _nextState();
  }
}

void Anton::_sleepSensors()
{
  if (!_airSensor)
  {
    _nextState();
    return;
  }

  if (_airSensor->sleep())
  {
    _nextState();
  }
}

void Anton::_report()
{
  AirData *ad = nullptr;
  EnvironmentData *ed = nullptr;
  CalculatedAQI *aqi = nullptr;
  CO2Data *co2 = nullptr;

  if (_airSensor && !_stateFailed[SAMPLE_PARTICULATE])
  {
    ad = &_airData;
    if (calculateAQI(_airData, &_aqi))
    {
      aqi = &_aqi;
    }
  }

  if (_environmentSensor && !_stateFailed[SAMPLE_ENVIRONMENT])
  {
    ed = &_environmentData;
  }

  if (_co2Sensor && !_stateFailed[SAMPLE_CO2])
  {
    co2 = &_co2Data;
  }

  if (_reporter->report(ad, aqi, ed, co2))
  {
    _lastErrorMessage = "";
    _lastReported = millis();

    _nextState();

    return;
  }

  _lastErrorMessage = _reporter->getLastErrorMessage();
  Serial.printf("failed to report: %s\n", _lastErrorMessage.c_str());
  _retry(3000);
}

void Anton::_sleep()
{
  Serial.println("sleeping");
  _nextState();
}
