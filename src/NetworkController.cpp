#include "NetworkController.h"
#include "strings.h"
#include "util.h"

NetworkController::NetworkController(const char *productName,
                                     const char *defaultPassword,
                                     DNSServer *dnsServer,
                                     WebServer *webServer)
    : _iotWebConf(productName, dnsServer, webServer, defaultPassword, CONFIG_VERSION)
{
  _sensorGroup.addItem(&_sensorName);
  _sensorGroup.addItem(&_sensorLocation);
  _iotWebConf.addParameterGroup(&_sensorGroup);

  _influxdbGroup.addItem(&_influxdbHost);
  _influxdbGroup.addItem(&_influxdbPort);
  _influxdbGroup.addItem(&_influxdbDatabase);
  _iotWebConf.addParameterGroup(&_influxdbGroup);

  _particleSensorGroup.addItem(&_particleSensorEnable);
  _particleSensorGroup.addItem(&_particleSensor);
  _iotWebConf.addParameterGroup(&_particleSensorGroup);

  _vocSensorGroup.addItem(&_vocSensorEnable);
  _vocSensorGroup.addItem(&_vocSensor);
  _iotWebConf.addParameterGroup(&_vocSensorGroup);

  _co2SensorGroup.addItem(&_co2SensorEnable);
  _co2SensorGroup.addItem(&_co2Sensor);
  _iotWebConf.addParameterGroup(&_co2SensorGroup);

  _iotWebConf.setWifiConnectionCallback([this]()
                                        { _wifiConnected(); });
  _iotWebConf.setWifiConnectionTimeoutMs(60000);

  _iotWebConf.setupUpdateServer(
      [this, webServer](const char *updatePath)
      { _httpUpdater.setup(webServer, updatePath); },
      [this](const char *userName, char *password)
      { _httpUpdater.updateCredentials(userName, password); });

  _iotWebConf.init();

  memcpy(_config.sensorName, _sensorName.value(), STRING_LEN);
  memcpy(_config.sensorLocation, _sensorLocation.value(), STRING_LEN);

  sprintf(_config.influxdbUrl, "http://%s:%d", _influxdbHost.value(), _influxdbPort.value());
  memcpy(_config.influxdbDatabase, _influxdbDatabase.value(), STRING_LEN);

  _config.particleSensorEnabled = _particleSensorEnable.value();
  if (strcmp(_particleSensor.value(), "zho3b") == 0)
  {
    _config.particleSensor = ParticleSensor::Winsen_ZH03B;
  }
  else if (strcmp(_particleSensor.value(), "pms7003") == 0)
  {
    _config.particleSensor = ParticleSensor::Plantower_PMS7003;
  }
  else
  {
    // TODO: handle failure
  }

  _config.vocSensorEnabled = _vocSensorEnable.value();
  if (strcmp(_vocSensor.value(), "bme680") == 0)
  {
    _config.vocSensor = VocSensor::Bosch_BME680;
  }
  else
  {
    // TODO: handle failure
  }

  _config.co2SensorEnabled = _co2SensorEnable.value();
  if (strcmp(_co2Sensor.value(), "mhz19b") == 0)
  {
    _config.co2Sensor = Co2Sensor::Winsen_MHZ19B;
  }
  else
  {
    // TODO: handle failure
  }
}

void NetworkController::loop()
{
  _iotWebConf.doLoop();
}

void NetworkController::_wifiConnected()
{
  Serial.println("setup: wifi connected");
}

void NetworkController::resetConfig(bool hard)
{
  if (hard)
  {
    util::clearEEPROM();
  }
  else
  {
    _iotWebConf.getSystemParameterGroup()->applyDefaultValue();
    _iotWebConf.getWifiParameterGroup()->applyDefaultValue();
    _sensorGroup.applyDefaultValue();
    _influxdbGroup.applyDefaultValue();
    _particleSensorGroup.applyDefaultValue();
    _vocSensorGroup.applyDefaultValue();
    _co2SensorGroup.applyDefaultValue();
    _iotWebConf.saveConfig();
  }
  ESP.restart();
}

void NetworkController::reboot()
{
  ESP.restart();
}

bool NetworkController::hasHandledCaptivePortal()
{
  return _iotWebConf.handleCaptivePortal();
}

void NetworkController::handleConfig()
{
  return _iotWebConf.handleConfig();
}

void NetworkController::handleNotFound()
{
  return _iotWebConf.handleNotFound();
}
