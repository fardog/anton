#ifndef Anton_h
#define Anton_h

#include <Arduino.h>

#include "reporters/Reporter.h"
#include "sensors/AirSensor.h"
#include "sensors/CO2Sensor.h"
#include "sensors/EnvironmentSensor.h"

enum StateId
{
  STARTUP = 0,
  WAKE_SENSORS,
  WARM_UP,
  SAMPLE_PARTICULATE,
  SAMPLE_CO2,
  SAMPLE_MISC,
  SLEEP_SENSORS,
  REPORT,
  SLEEP
};
const static uint16_t numStates = 9;

struct State
{
  StateId state;
  uint16_t delay;
  uint16_t timeout;
  StateId next;
};

class Anton
{
public:
  Anton(Reporter *reporter,
        AirSensor *airSensor,
        CO2Sensor *co2Sensor,
        EnvironmentSensor *environmenSensor,
        uint16_t timeBetweenMeasurements = 30000);
  ~Anton();

  void loop();
  AirData airData() { return _airData; }
  CO2Data co2Data() { return _co2Data; }
  EnvironmentData environmentData() { return _environmentData; }
  CalculatedAQI aqi() { return _aqi; }
  unsigned long lastReported() { return _lastReported; }
  String lastErrorMessage() { return _lastErrorMessage; }

private:
  Reporter *_reporter;
  AirSensor *_airSensor;
  CO2Sensor *_co2Sensor;
  EnvironmentSensor *_environmentSensor;

  const State _states[numStates];

  State _state;

  void _startup();
  void _wakeSensors();
  void _warmUp();
  void _sampleParticulate();
  void _sampleCO2();
  void _sleepSensors();
  void _sampleMisc();
  void _report();
  void _sleep();

  void _nextState();
  void _retry(uint16_t delay);

  unsigned long _stateStart{0};
  unsigned long _lastTransition{0};
  unsigned long _lastReported{0};
  AirData _airData{};
  CO2Data _co2Data{};
  EnvironmentData _environmentData{};
  CalculatedAQI _aqi{};
  String _lastErrorMessage{""};
  bool _stateFailed[numStates]{};
};

#endif
