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

#include "strings.h"

#include "Anton.h"
#include "reporters/Reporter.h"
#include "reporters/InfluxDB_Reporter.h"
#include "sensors/AirSensor.h"
#include "sensors/ZH03B_AirSensor.h"
#include "sensors/PMS_AirSensor.h"
#include "sensors/EnvironmentSensor.h"
#include "sensors/BME680_EnvironmentSensor.h"
#include "sensors/CO2Sensor.h"
#include "sensors/MHZ19B_CO2Sensor.h"
#include "util.h"

#ifndef GIT_REV
#define GIT_REV "Unknown"
#endif

#define CONFIG_VERSION "v2.2"
#define PRODUCT_NAME "anton"
#define DEFAULT_PASSWORD "anton-system"

#define STRING_LEN 128

// constants initialized at setup
char influxdbURL[200] = "";

// callbacks
void wifiConnected();

DNSServer dnsServer;
WebServer server(80);

#ifdef ESP8266
ESP8266HTTPUpdateServer httpUpdater;
#elif defined(ESP32)
HTTPUpdateServer httpUpdater;
#endif

IotWebConf iotWebConf(PRODUCT_NAME, &dnsServer, &server, DEFAULT_PASSWORD, CONFIG_VERSION);

iotwebconf::ParameterGroup sensorGroup = iotwebconf::ParameterGroup("sensorGroup", "Reporting");
iotwebconf::TextTParameter<STRING_LEN> sensorName =
    iotwebconf::Builder<iotwebconf::TextTParameter<STRING_LEN>>("sensorName")
        .label("Sensor Name")
        .defaultValue("anton-default")
        .build();
iotwebconf::TextTParameter<STRING_LEN> sensorLocation =
    iotwebconf::Builder<iotwebconf::TextTParameter<STRING_LEN>>("sensorLocation")
        .label("Sensor Location")
        .defaultValue("")
        .build();

iotwebconf::ParameterGroup influxdbGroup = iotwebconf::ParameterGroup("influxdbGroup", "InfluxDB");
iotwebconf::TextTParameter<STRING_LEN> influxdbHost =
    iotwebconf::Builder<iotwebconf::TextTParameter<STRING_LEN>>("influxdbHost")
        .label("Host")
        .defaultValue("influxdb")
        .build();
iotwebconf::IntTParameter<uint16_t> influxdbPort =
    iotwebconf::Builder<iotwebconf::IntTParameter<uint16_t>>("influxdbPort")
        .label("Port")
        .defaultValue(8086)
        .min(1)
        .max(65535)
        .step(1)
        .placeholder("8086")
        .build();
iotwebconf::TextTParameter<STRING_LEN> influxdbDatabase =
    iotwebconf::Builder<iotwebconf::TextTParameter<STRING_LEN>>("influxdbDatabase")
        .label("Database Name")
        .defaultValue("anton")
        .build();

static const char particleSensorValues[][STRING_LEN] = {"zh03b", "pms7003"};
static const char particleSensorNames[][STRING_LEN] = {"Winsen ZH03B", "Plantower PMS7003"};
#ifdef ESP8266
static const char particleSensorRXValues[][4] = {"D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10"};
static const char particleSensorRXNames[][STRING_LEN] = {"D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10"};
static const char particleSensorTXValues[][4] = {"D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10"};
#elif defined(ESP32)
static const char particleSensorRXValues[][4] = {"U0", "U2"};
static const char particleSensorRXNames[][STRING_LEN] = {
    "UART0(rx:GPIO3,tx:GPIO1)",
    "UART2(rx:GPIO16,tx:GPIO17)"};
static const char particleSensorTXValues[][4] = {"--"};
#endif
iotwebconf::ParameterGroup particleSensorGroup = iotwebconf::ParameterGroup("particleSensorGroup", "Particulate Sensor");
iotwebconf::CheckboxTParameter particleSensorEnable =
    iotwebconf::Builder<iotwebconf::CheckboxTParameter>("particleSensorEnable")
        .label("Enabled")
        .defaultValue(false)
        .build();
iotwebconf::SelectTParameter<STRING_LEN> particleSensor =
    iotwebconf::Builder<iotwebconf::SelectTParameter<STRING_LEN>>("particleSensor")
        .label("Sensor")
        .optionValues((const char *)particleSensorValues)
        .optionNames((const char *)particleSensorNames)
        .optionCount(sizeof(particleSensorValues) / STRING_LEN)
        .nameLength(STRING_LEN)
        .defaultValue("zh03b")
        .build();
iotwebconf::SelectTParameter<4> particleSensorRX =
    iotwebconf::Builder<iotwebconf::SelectTParameter<4>>("particleSensorRX")
#ifdef ESP8266
        .label("RX Pin")
#elif defined(ESP32)
        .label("UART")
#endif
        .optionValues((const char *)particleSensorRXValues)
        .optionNames((const char *)particleSensorRXNames)
        .optionCount(sizeof(particleSensorRXValues) / 4)
        .nameLength(STRING_LEN)
#ifdef ESP8266
        .defaultValue("D3")
#elif defined(ESP32)
        .defaultValue("U2")
#endif
        .build();
iotwebconf::SelectTParameter<4> particleSensorTX =
    iotwebconf::Builder<iotwebconf::SelectTParameter<4>>("particleSensorTX")
        .label("TX Pin")
        .optionValues((const char *)particleSensorTXValues)
        .optionNames((const char *)particleSensorTXValues)
        .optionCount(sizeof(particleSensorTXValues) / 4)
        .nameLength(4)
#ifdef ESP8266
        .defaultValue("D4")
#elif defined(ESP32)
        .defaultValue("--")
#endif
        .build();

static const char vocSensorValues[][STRING_LEN] = {"bme680"};
static const char vocSensorNames[][STRING_LEN] = {"Bosch BME680"};
iotwebconf::ParameterGroup vocSensorGroup = iotwebconf::ParameterGroup("vocSensorGroup", "VOC Sensor");
iotwebconf::CheckboxTParameter vocSensorEnable =
    iotwebconf::Builder<iotwebconf::CheckboxTParameter>("vocSensorEnable")
        .label("Enabled")
        .defaultValue(false)
        .build();
iotwebconf::SelectTParameter<STRING_LEN> vocSensor =
    iotwebconf::Builder<iotwebconf::SelectTParameter<STRING_LEN>>("vocSensor")
        .label("Sensor")
        .optionValues((const char *)vocSensorValues)
        .optionNames((const char *)vocSensorNames)
        .optionCount(sizeof(vocSensorValues) / STRING_LEN)
        .nameLength(STRING_LEN)
        .defaultValue("zh03b")
        .build();

iotwebconf::ParameterGroup co2SensorGroup = iotwebconf::ParameterGroup("co2SensorGroup", "CO2 Sensor");
iotwebconf::CheckboxTParameter co2SensorEnable =
    iotwebconf::Builder<iotwebconf::CheckboxTParameter>("co2SensorEnable")
        .label("Enabled")
        .defaultValue(false)
        .build();

Stream *airSensorSerial = nullptr;
AirSensor *airSensor = nullptr;
Reporter *reporter = nullptr;
EnvironmentSensor *environmentSensor = nullptr;
Stream *co2SensorSerial = nullptr;
CO2Sensor *co2Sensor = nullptr;
Anton *anton = nullptr;

void resetAll()
{
  iotWebConf.getSystemParameterGroup()->applyDefaultValue();
  iotWebConf.getWifiParameterGroup()->applyDefaultValue();
  sensorGroup.applyDefaultValue();
  influxdbGroup.applyDefaultValue();
  particleSensorGroup.applyDefaultValue();
  vocSensorGroup.applyDefaultValue();
  co2SensorGroup.applyDefaultValue();
  iotWebConf.saveConfig();
  ESP.restart();
}

void hardReset()
{
  util::clearEEPROM();
  ESP.restart();
}

void renderIndexPage(char *buf, Anton *anton)
{
  AirData ad = anton->airData();
  EnvironmentData ed = anton->environmentData();
  CalculatedAQI aqi = anton->aqi();
  CO2Data co2 = anton->co2Data();

  int lastMeasured = -1;
  if (ad.timestamp > 0)
  {
    lastMeasured = (millis() - ad.timestamp) / 1000;
  }

  int reported = -1;
  if (anton->lastReported() > 0)
  {
    reported = (millis() - anton->lastReported()) / 1000;
  }

  String lastStatus = anton->lastErrorMessage();
  if (lastStatus.equals(""))
  {
    lastStatus = "SUCCESS";
  }

  sprintf(
      buf,
      serverIndex,
      lastMeasured,
      reported,
      lastStatus.c_str(),
      ad.p1_0,
      ad.p2_5,
      ad.p10_0,
      util::rnd(aqi.value),
      aqi.pollutant,
      util::rnd(ed.tempC),
      util::rnd(ed.humPct),
      util::rnd(ed.iaq),
      co2.ppm,
      millis() / 1000,
      sensorName.value(),
      influxdbURL,
      GIT_REV);
}

void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }

  if (anton)
  {
    char buf[2048];
    renderIndexPage(buf, anton);
    server.send(200, "text/html", buf);
  }
  else
  {
    server.send(200, "text/html", serverUnconfigured);
  }
}

void handleGetCalibrate()
{
  server.send(200, "text/html", calibrationPage);
}

void handlePostCalibrate()
{
  String co2Success = String("NOT PRESENT");
  if (co2Sensor)
  {
    if (co2Sensor->calibrate())
    {
      co2Success = String("REQUESTED");
    }
    else
    {
      co2Success = String("FAILED");
    }
  }

  char buf[2048];
  sprintf(buf, calibrationResultPage, co2Success.c_str());
  server.send(200, "text/html", buf);
}

void setup()
{
  Serial.begin(115200);
  Serial.printf("setup: starting version %s\n", GIT_REV);

  sensorGroup.addItem(&sensorName);
  sensorGroup.addItem(&sensorLocation);
  iotWebConf.addParameterGroup(&sensorGroup);

  influxdbGroup.addItem(&influxdbHost);
  influxdbGroup.addItem(&influxdbPort);
  influxdbGroup.addItem(&influxdbDatabase);
  iotWebConf.addParameterGroup(&influxdbGroup);

  particleSensorGroup.addItem(&particleSensorEnable);
  particleSensorGroup.addItem(&particleSensor);
  particleSensorGroup.addItem(&particleSensorRX);
  particleSensorGroup.addItem(&particleSensorTX);
  iotWebConf.addParameterGroup(&particleSensorGroup);

  vocSensorGroup.addItem(&vocSensorEnable);
  vocSensorGroup.addItem(&vocSensor);
  iotWebConf.addParameterGroup(&vocSensorGroup);

  co2SensorGroup.addItem(&co2SensorEnable);
  iotWebConf.addParameterGroup(&co2SensorGroup);

  iotWebConf.setWifiConnectionCallback(&wifiConnected);
  iotWebConf.setWifiConnectionTimeoutMs(60000);

  iotWebConf.setupUpdateServer(
      [](const char *updatePath)
      { httpUpdater.setup(&server, updatePath); },
      [](const char *userName, char *password)
      { httpUpdater.updateCredentials(userName, password); });

  iotWebConf.init();

  server.on("/", handleRoot);
  server.on("/config", []
            { iotWebConf.handleConfig(); });
  server.onNotFound([]()
                    { iotWebConf.handleNotFound(); });

  server.on("/calibrate", handleGetCalibrate);
  server.on("/calibrate-confirm", HTTP_POST, handlePostCalibrate);
  server.on("/reset", HTTP_GET, []()
            {
              Serial.println("http: serving reset");
              server.sendHeader("Connection", "close");
              server.send(200, "text/html", resetPage);
            });
  server.on("/reboot", HTTP_POST, []()
            {
              Serial.println("http: serving reboot");
              server.sendHeader("Connection", "close");
              server.send(200, "text/plain", "rebooting");
              ESP.restart();
            });
  server.on("/reset-confirm", HTTP_POST, []()
            {
              Serial.println("http: serving reset confirm");
              server.sendHeader("Connection", "close");
              server.send(200, "text/plain", "resetting");
              resetAll();
            });
  server.on("/hard-reset-confirm", HTTP_POST, []()
            {
              Serial.println("http: serving hard reset confirm");
              server.sendHeader("Connection", "close");
              server.send(200, "text/plain", "resetting");
              hardReset();
            });

  sprintf(influxdbURL, "http://%s:%d", influxdbHost.value(), influxdbPort.value());
  Serial.printf("setup: influxdb url: %s\n", influxdbURL);

  // set up reporter
  InfluxDB_Reporter *DB = new InfluxDB_Reporter(sensorName.value(),
                                                sensorLocation.value(),
                                                influxdbURL,
                                                influxdbDatabase.value());
  reporter = DB;

  if (particleSensorEnable.value())
  {
#ifdef ESP8266
    airSensorSerial = util::getSerial(particleSensorRX.value(), particleSensorTX.value());
#elif defined(ESP32)
    airSensorSerial = util::getSerial(particleSensorRX.value());
#endif

    if (strcmp(particleSensor.value(), "zh03b") == 0)
    {
      Serial.printf("setup: starting ZH03B particle sensor on serial: %s, %s\n",
                    particleSensorRX.value(), particleSensorTX.value());
      ZH03B_AirSensor *ZH = new ZH03B_AirSensor(*airSensorSerial);
      airSensor = ZH;
    }
    else if (strcmp(particleSensor.value(), "pms7003") == 0)
    {
      Serial.printf("setup: starting PMS7003 particle sensor on serial: %s, %s\n",
                    particleSensorRX.value(), particleSensorTX.value());
      PMS_AirSensor *PMS = new PMS_AirSensor(*airSensorSerial);
      airSensor = PMS;
    }
    else
    {
      Serial.printf("setup: invalid particle sensor type: %s\n", particleSensor.value());
    }
  }

  if (vocSensorEnable.value())
  {
    Serial.println("setup: starting BME680 VOC sensor on I2C (default pins)");
    BME680_EnvironmentSensor *BME = new BME680_EnvironmentSensor(320, 150);
    environmentSensor = BME;
  }

  if (co2SensorEnable.value())
  {
    // TODO: make esp8266 correct
#ifdef ESP8266
    co2SensorSerial = util::getSerial(particleSensorRX.value(), particleSensorTX.value());
#elif defined(ESP32)
    co2SensorSerial = util::getSerial("U0");
#endif
    co2Sensor = new MHZ19B_CO2Sensor(co2SensorSerial);
  }

  anton = new Anton(reporter, airSensor, co2Sensor, environmentSensor);
}

void wifiConnected()
{
  Serial.println("setup: wifi connected");
}

void loop()
{
  iotWebConf.doLoop();
  anton->loop();
}
