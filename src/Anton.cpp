#include <Arduino.h>
#include "Anton.h"

Anton::Anton(Reporter *reporter, AirSensor *airSensor, EnvironmentSensor *environmentSensor)
    : _reporter(reporter),
      _airSensor(airSensor),
      _environmentSensor(environmentSensor),
      _states{
          {StateId::STARTUP, 0, StateId::WAKE_SENSORS},
          {StateId::WAKE_SENSORS, 0, StateId::WARM_UP},
          {StateId::WARM_UP, 10000, StateId::SAMPLE},
          {StateId::SAMPLE, 0, StateId::SLEEP_SENSORS},
          {StateId::SLEEP_SENSORS, 0, StateId::REPORT},
          {StateId::REPORT, 0, StateId::SLEEP},
          {StateId::SLEEP, 30000, StateId::WAKE_SENSORS},
      }
{
  _state = _states[0];
}

Anton::~Anton() {}

void Anton::loop()
{
  if (millis() < _delayUntil)
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
  case SAMPLE:
    _sample();
    break;
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
  Serial.printf("state change: %d -> %d\n", _state.state, _state.next);
  _delayUntil = millis() + _state.delay;
  _state = _states[_state.next];
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

void Anton::_sample()
{
  if (_airSensor)
  {
    _airSensor->loop();
    AirData airSample;
    if (_airSensor->getAirData(&airSample))
    {
      Serial.printf("Particulate: PM1.0, PM2.5, PM10=[%d %d %d]\n",
                    airSample.p1_0,
                    airSample.p2_5,
                    airSample.p10_0);
      // lastAirData = airSample;
      // lastMeasured = millis();
      // strcpy(lastStatus, "SUCCESS");

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
  _nextState();
}

void Anton::_sleep()
{
  Serial.println("sleeping");
  _nextState();
}
