#ifndef Anton_h
#define Anton_h

#include <Arduino.h>

#include "reporters/Reporter.h"
#include "sensors/AirSensor.h"
#include "sensors/EnvironmentSensor.h"

enum StateId
{
  STARTUP = 0,
  WAKE_SENSORS,
  WARM_UP,
  SAMPLE,
  SLEEP_SENSORS,
  REPORT,
  SLEEP
};
const static uint16_t numStates = 7;

struct State
{
  StateId state;
  uint16_t delay;
  StateId next;
};

class Anton
{
public:
  Anton(Reporter *reporter, AirSensor *airSensor, EnvironmentSensor *environmenSensor);
  ~Anton();

  void loop();

private:
  Reporter *_reporter;
  AirSensor *_airSensor;
  EnvironmentSensor *_environmentSensor;

  const State _states[numStates];

  State _state;

  void _startup();
  void _wakeSensors();
  void _warmUp();
  void _sample();
  void _sleepSensors();
  void _report();
  void _sleep();

  void _nextState();

  unsigned long _delayUntil = 0;
};

#endif
