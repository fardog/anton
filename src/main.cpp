#include <Arduino.h>
#include <SoftwareSerial.h>

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

#include "reporters/Reporter.h"
#include "reporters/InfluxDB_Reporter.h"
#include "sensors/AirSensor.h"
#include "sensors/ZH03B_AirSensor.h"
#include "sensors/EnvironmentSensor.h"
#include "sensors/BME680_EnvironmentSensor.h"
#include "util.h"

#ifndef GIT_REV
#define GIT_REV "Unknown"
#endif

#define CONFIG_VERSION "v2.1"
#define PRODUCT_NAME "anton"
#define DEFAULT_PASSWORD "anton-system"

#define STRING_LEN 128

// global variables
bool ready = false;
int wakeupFailCounter = 0;
int lastMeasured = 0;
AirData lastAirData;
EnvironmentData lastEnvironmentData;
char lastStatus[10] = "NONE";
int lastAQI = 0;
char lastPrimaryContributor[5] = "NONE";

// constants
static const int NUM_SAMPLES = 10;
static const int MIN_SAMPLES_FOR_SUCCESS = 4;
static const int SENSOR_STARTUP_DELAY = 10000;
static const int SAMPLE_DELAY = 3000;
static const int MEASUREMENT_DELAY = 60000;
static const int MAX_JSON_DOCUMENT_SIZE = 2048;
static const int MAX_SENSOR_WAKE_FAIL = 5;
static const int MAX_CONNECTION_FAIL = 5;

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

static const char particleSensorValues[][STRING_LEN] = {"zh03b"};
static const char particleSensorNames[][STRING_LEN] = {"Winsen ZH03B"};
static const char particleSensorRXValues[][4] = {"D1", "D3"};
static const char particleSensorTXValues[][4] = {"D2", "D4"};
iotwebconf::ParameterGroup particleSensorGroup = iotwebconf::ParameterGroup("particleSensorGroup", "Particulate Sensor");
iotwebconf::CheckboxTParameter particleSensorEnable =
    iotwebconf::Builder<iotwebconf::CheckboxTParameter>("particleSensorEnable")
        .label("Enabled")
        .defaultValue(true)
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
        .label("RX Pin")
        .optionValues((const char *)particleSensorRXValues)
        .optionNames((const char *)particleSensorRXValues)
        .optionCount(sizeof(particleSensorRXValues) / 4)
        .nameLength(4)
        .defaultValue("D3")
        .build();
iotwebconf::SelectTParameter<4> particleSensorTX =
    iotwebconf::Builder<iotwebconf::SelectTParameter<4>>("particleSensorTX")
        .label("TX Pin")
        .optionValues((const char *)particleSensorTXValues)
        .optionNames((const char *)particleSensorTXValues)
        .optionCount(sizeof(particleSensorTXValues) / 4)
        .nameLength(4)
        .defaultValue("D4")
        .build();

static const char vocSensorValues[][STRING_LEN] = {"bme680"};
static const char vocSensorNames[][STRING_LEN] = {"Bosch BME680"};
iotwebconf::ParameterGroup vocSensorGroup = iotwebconf::ParameterGroup("vocSensorGroup", "VOC Sensor");
iotwebconf::CheckboxTParameter vocSensorEnable =
    iotwebconf::Builder<iotwebconf::CheckboxTParameter>("vocSensorEnable")
        .label("Enabled")
        .defaultValue(true)
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

SoftwareSerial *airSensorSerial;
AirSensor *airSensor = nullptr;
Reporter *reporter = nullptr;
EnvironmentSensor *environmentSensor;

void _delay(unsigned long ms)
{
  const int now = millis();

  while (1)
  {
    iotWebConf.doLoop();
    delay(100);
    if (millis() - now > ms)
    {
      break;
    }
  }
}

void resetAll()
{
  iotWebConf.getSystemParameterGroup()->applyDefaultValue();
  iotWebConf.getWifiParameterGroup()->applyDefaultValue();
  sensorGroup.applyDefaultValue();
  influxdbGroup.applyDefaultValue();
  particleSensorGroup.applyDefaultValue();
  vocSensorGroup.applyDefaultValue();
  iotWebConf.saveConfig();
  ESP.restart();
}

void hardReset()
{
  util::clearEEPROM();
  ESP.reset();
}

void renderIndexPage(char *buf)
{
  int last = -1;
  if (lastMeasured > 0)
  {
    last = (millis() - lastMeasured) / 1000;
  }
  sprintf(
      buf,
      serverIndex,
      last,
      lastStatus,
      util::rnd(lastEnvironmentData.tempC),
      util::rnd(lastEnvironmentData.humPct),
      util::rnd(lastEnvironmentData.iaq),
      lastAQI,
      lastPrimaryContributor,
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

  char buf[2048];
  renderIndexPage(buf);

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
    airSensorSerial = new SoftwareSerial(
        strcmp(particleSensorRX.value(), "D1") ? D1 : D3,
        strcmp(particleSensorTX.value(), "D2") ? D2 : D4);
    airSensorSerial->begin(9600);
    ZH03B_AirSensor *ZH = new ZH03B_AirSensor(*airSensorSerial);
    airSensor = ZH;
  }

  if (vocSensorEnable.value())
  {
    BME680_EnvironmentSensor *BME = new BME680_EnvironmentSensor(320, 150);
    environmentSensor = BME;
  }
}

void wifiConnected()
{
  Serial.println("setup: wifi connected");
  ready = true;
}

int sortUint16Desc(const void *cmp1, const void *cmp2)
{
  uint16_t a = *((uint16_t *)cmp1);
  uint16_t b = *((uint16_t *)cmp2);
  return b - a;
}

uint16_t medianValue(uint16_t *values, int count)
{
  qsort(values, count, sizeof(values[0]), sortUint16Desc);

  return values[(count / 2) - 1];
}

bool sampleParticleSensor(AirData *sample)
{
  // wake sensor
  if (airSensor->wake())
  {
    Serial.println("loop: sensor wakeup successful");
    wakeupFailCounter = 0;
  }
  else if (wakeupFailCounter >= MAX_SENSOR_WAKE_FAIL)
  {
    Serial.printf("loop: ERROR failed to wake sensor %d times; resetting", wakeupFailCounter);
    ESP.restart();
    return false;
  }
  else
  {
    Serial.println("loop: ERROR failed to wake the sensor; delaying and trying again");
    wakeupFailCounter += 1;
    _delay(30000);
    return false;
  }

  // delay after sensor start; waking the sensor starts its fan, and we want to
  // wait for a period before actually sampling it.
  _delay(SENSOR_STARTUP_DELAY);

  Serial.println("sensor: attemping average sample");
  uint16_t pm1_0[NUM_SAMPLES];
  uint16_t pm2_5[NUM_SAMPLES];
  uint16_t pm10_0[NUM_SAMPLES];
  int successes = 0;

  AirData buf;
  for (int i = 0; i < NUM_SAMPLES; i++)
  {
    if (airSensor->getAirData(&buf))
    {
      if (buf.p1_0 == buf.p2_5 && buf.p2_5 == buf.p10_0 && buf.p1_0 > 500)
      {
        Serial.println("sensor: faulty measurement, all values are equal and very large. discarding");
      }
      else
      {
        pm1_0[successes] = buf.p1_0;
        pm2_5[successes] = buf.p2_5;
        pm10_0[successes] = buf.p10_0;
        successes++;

        Serial.printf("sensor: sample PM1.0, PM2.5, PM10=[%d, %d, %d]\n", buf.p1_0, buf.p2_5, buf.p10_0);
      }
    }
    _delay(SAMPLE_DELAY);
  }

  // shut down sensor, stopping its fan.
  if (airSensor->sleep())
  {
    Serial.println("loop: sensor sleep successful");
  }
  else
  {
    Serial.println("loop: ERROR failed to sleep the sensor");
  }

  if (successes < MIN_SAMPLES_FOR_SUCCESS)
  {
    Serial.println("sensor: failed to get minimum number of samples");
    return false;
  }

  sample->p1_0 = medianValue(pm1_0, successes);
  sample->p2_5 = medianValue(pm2_5, successes);
  sample->p10_0 = medianValue(pm10_0, successes);

  return true;
}

void loop()
{
  iotWebConf.doLoop();
  if (!ready)
  {
    return;
  }

  EnvironmentData envSample;
  bool envSuccess = false;
  if (environmentSensor)
  {
    envSuccess = environmentSensor->getEnvironmentData(&envSample);
    if (envSuccess)
    {
      lastEnvironmentData = envSample;
    }
  }

  AirData airSample;
  bool airSuccess = false;
  if (airSensor)
  {
    airSuccess = sampleParticleSensor(&airSample);
    if (airSuccess)
    {
      Serial.printf("Aggregate: PM1.0, PM2.5, PM10=[%d %d %d]\n",
                    airSample.p1_0,
                    airSample.p2_5,
                    airSample.p10_0);
      lastAirData = airSample;
      lastMeasured = millis();
      strcpy(lastStatus, "SUCCESS");
    }
    else
    {
      Serial.println("loop: failed to sample sensor");
      strcpy(lastStatus, "FAILURE");
    }
  }

  CalculatedAQI aqi;
  bool aqiSuccess = false;

  if (airSuccess)
  {
    aqiSuccess = calculateAQI(airSample, &aqi);
  }

  if (reporter->report(airSuccess ? &airSample : nullptr, aqiSuccess ? &aqi : nullptr, envSuccess ? &envSample : nullptr))
  {
    Serial.println("loop: sample submitted successfully");
  }
  else
  {
    Serial.println("loop: failed to submit sample");
    Serial.println(reporter->getLastErrorMessage());
  }

  if (aqiSuccess)
  {
    lastAQI = aqi.value;
    strcpy(lastPrimaryContributor, aqi.pollutant);
  }
  else
  {
    lastAQI = -1;
    strcpy(lastPrimaryContributor, "NONE");
  }

  _delay(MEASUREMENT_DELAY);
}
