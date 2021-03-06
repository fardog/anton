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

#include "reporters/Reporter.h"
#include "reporters/InfluxDB_Reporter.h"
#include "sensors/AirSensor.h"
#include "sensors/ZH03B_AirSensor.h"
#include "sensors/PMS_AirSensor.h"
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

static const char particleSensorValues[][STRING_LEN] = {"zh03b", "pms7003"};
static const char particleSensorNames[][STRING_LEN] = {"Winsen ZH03B", "Plantower PMS7003"};
#ifdef ESP8266
static const char particleSensorRXValues[][4] = {"D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10"};
static const char particleSensorRXNames[][STRING_LEN] = {"D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10"};
static const char particleSensorTXValues[][4] = {"D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10"};
#elif defined(ESP32)
static const char particleSensorRXValues[][4] = {"U0", "U1", "U2"};
static const char particleSensorRXNames[][STRING_LEN] = {
    "UART0(rx:GPIO3,tx:GPIO1)",
    "UART1(rx:GPIO9,tx:GPIO10)",
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

Stream *airSensorSerial;
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
  ESP.restart();
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
      lastAirData.p1_0,
      lastAirData.p2_5,
      lastAirData.p10_0,
      lastAQI,
      lastPrimaryContributor,
      util::rnd(lastEnvironmentData.tempC),
      util::rnd(lastEnvironmentData.humPct),
      util::rnd(lastEnvironmentData.iaq),
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
}

void wifiConnected()
{
  Serial.println("setup: wifi connected");
  ready = true;
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
    Serial.printf("loop: ERROR failed to wake sensor %d times; resetting\n", wakeupFailCounter);
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

  Serial.println("sensor: attemping aggregate sample");
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

  sample->p1_0 = util::medianValue(pm1_0, successes);
  sample->p2_5 = util::medianValue(pm2_5, successes);
  sample->p10_0 = util::medianValue(pm10_0, successes);

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
      Serial.printf("Environment: %.2f°C, %.2f%%rh, IAQ %.2f, %.2fhPa, %.2fOhm\n",
                    envSample.tempC,
                    envSample.humPct,
                    envSample.iaq,
                    envSample.pressure,
                    envSample.gasResistance);
      lastEnvironmentData = envSample;
    }
    else
    {
      Serial.println("loop: failed to sample environment sensor");
    }
  }

  AirData airSample;
  bool airSuccess = false;
  if (airSensor)
  {
    airSuccess = sampleParticleSensor(&airSample);
    if (airSuccess)
    {
      Serial.printf("Particulate: PM1.0, PM2.5, PM10=[%d %d %d]\n",
                    airSample.p1_0,
                    airSample.p2_5,
                    airSample.p10_0);
      lastAirData = airSample;
      lastMeasured = millis();
      strcpy(lastStatus, "SUCCESS");
    }
    else
    {
      Serial.println("loop: failed to sample particulate sensor");
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
    Serial.printf("loop: failed to submit sample, reason: %s\n", reporter->getLastErrorMessage().c_str());
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
