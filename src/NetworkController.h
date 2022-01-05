#ifndef NetworkController_h
#define NetworkController_h

#include <Arduino.h>

#include <AQI.h>

#include <IotWebConf.h>
#include <IotWebConfTParameter.h>

#ifdef ESP8266
#include <ESP8266HTTPUpdateServer.h>
#elif defined(ESP32)
// For ESP32 IotWebConf provides a drop-in replacement for UpdateServer.
#include <IotWebConfESP32HTTPUpdateServer.h>
#endif

#include "SensorController.h"
#include "interfaces/Looper.h"

#define STRING_LEN 128
#define CONFIG_VERSION "v3.0"

enum ParticleSensor
{
  Winsen_ZH03B,
  Plantower_PMS7003
};

enum VocSensor
{
  Bosch_BME680
};

enum Co2Sensor
{
  Winsen_MHZ19B
};

struct AntonConfiguration
{
  char sensorName[STRING_LEN];
  char sensorLocation[STRING_LEN];

  char influxdbUrl[STRING_LEN + 24];
  char influxdbDatabase[STRING_LEN];

  bool particleSensorEnabled;
  ParticleSensor particleSensor;

  bool vocSensorEnabled;
  VocSensor vocSensor;

  bool co2SensorEnabled;
  Co2Sensor co2Sensor;
};

class NetworkController : public Looper
{
public:
  NetworkController(const char *productName,
                    const char *defaultPassword,
                    DNSServer *dnsServer,
                    WebServer *webServer);

  void loop();
  void resetConfig(bool hard = false);
  void reboot();

  AntonConfiguration getConfig() { return _config; }

  bool hasHandledCaptivePortal();

  void handleConfig();
  void handleNotFound();

private:
  void _wifiConnected();

  IotWebConf _iotWebConf;

#ifdef ESP8266
  ESP8266HTTPUpdateServer _httpUpdater;
#elif defined(ESP32)
  HTTPUpdateServer _httpUpdater;
#endif

  AntonConfiguration _config;

  iotwebconf::ParameterGroup _sensorGroup = iotwebconf::ParameterGroup("sensorGroup", "Reporting");
  iotwebconf::TextTParameter<STRING_LEN> _sensorName =
      iotwebconf::Builder<iotwebconf::TextTParameter<STRING_LEN>>("sensorName")
          .label("Sensor Name")
          .defaultValue("anton-default")
          .build();
  iotwebconf::TextTParameter<STRING_LEN> _sensorLocation =
      iotwebconf::Builder<iotwebconf::TextTParameter<STRING_LEN>>("sensorLocation")
          .label("Sensor Location")
          .defaultValue("")
          .build();

  iotwebconf::ParameterGroup _influxdbGroup = iotwebconf::ParameterGroup("influxdbGroup", "InfluxDB");
  iotwebconf::TextTParameter<STRING_LEN> _influxdbHost =
      iotwebconf::Builder<iotwebconf::TextTParameter<STRING_LEN>>("influxdbHost")
          .label("Host")
          .defaultValue("influxdb")
          .build();
  iotwebconf::IntTParameter<uint16_t> _influxdbPort =
      iotwebconf::Builder<iotwebconf::IntTParameter<uint16_t>>("influxdbPort")
          .label("Port")
          .defaultValue(8086)
          .min(1)
          .max(65535)
          .step(1)
          .placeholder("8086")
          .build();
  iotwebconf::TextTParameter<STRING_LEN> _influxdbDatabase =
      iotwebconf::Builder<iotwebconf::TextTParameter<STRING_LEN>>("influxdbDatabase")
          .label("Database Name")
          .defaultValue("anton")
          .build();

  iotwebconf::ParameterGroup _particleSensorGroup = iotwebconf::ParameterGroup("particleSensorGroup", "Particulate Sensor");
  iotwebconf::CheckboxTParameter _particleSensorEnable =
      iotwebconf::Builder<iotwebconf::CheckboxTParameter>("particleSensorEnable")
          .label("Enabled")
          .defaultValue(false)
          .build();
  const char _particleSensorValues[2][STRING_LEN] = {"zh03b", "pms7003"};
  const char _particleSensorNames[2][STRING_LEN] = {"Winsen ZH03B", "Plantower PMS7003"};
  iotwebconf::SelectTParameter<STRING_LEN> _particleSensor =
      iotwebconf::Builder<iotwebconf::SelectTParameter<STRING_LEN>>("particleSensor")
          .label("Sensor")
          .optionValues((const char *)_particleSensorValues)
          .optionNames((const char *)_particleSensorNames)
          .optionCount(sizeof(_particleSensorValues) / STRING_LEN)
          .nameLength(STRING_LEN)
          .defaultValue("zh03b")
          .build();

  iotwebconf::ParameterGroup _vocSensorGroup = iotwebconf::ParameterGroup("vocSensorGroup", "VOC Sensor");
  iotwebconf::CheckboxTParameter _vocSensorEnable =
      iotwebconf::Builder<iotwebconf::CheckboxTParameter>("vocSensorEnable")
          .label("Enabled")
          .defaultValue(false)
          .build();
  const char _vocSensorValues[1][STRING_LEN] = {"bme680"};
  const char _vocSensorNames[1][STRING_LEN] = {"Bosch BME680"};
  iotwebconf::SelectTParameter<STRING_LEN> _vocSensor =
      iotwebconf::Builder<iotwebconf::SelectTParameter<STRING_LEN>>("vocSensor")
          .label("Sensor")
          .optionValues((const char *)_vocSensorValues)
          .optionNames((const char *)_vocSensorNames)
          .optionCount(sizeof(_vocSensorValues) / STRING_LEN)
          .nameLength(STRING_LEN)
          .defaultValue("zh03b")
          .build();

  iotwebconf::ParameterGroup _co2SensorGroup = iotwebconf::ParameterGroup("co2SensorGroup", "CO2 Sensor");
  iotwebconf::CheckboxTParameter _co2SensorEnable =
      iotwebconf::Builder<iotwebconf::CheckboxTParameter>("co2SensorEnable")
          .label("Enabled")
          .defaultValue(false)
          .build();
  const char _co2SensorValues[1][STRING_LEN] = {"mhz19b"};
  const char _co2SensorNames[1][STRING_LEN] = {"Winsen MH-Z19B"};
  iotwebconf::SelectTParameter<STRING_LEN> _co2Sensor =
      iotwebconf::Builder<iotwebconf::SelectTParameter<STRING_LEN>>("co2SensorSelect")
          .label("Sensor")
          .optionValues((const char *)_co2SensorValues)
          .optionNames((const char *)_co2SensorNames)
          .optionCount(sizeof(_co2SensorValues) / STRING_LEN)
          .nameLength(STRING_LEN)
          .defaultValue("mhz19b")
          .build();
};

#endif
