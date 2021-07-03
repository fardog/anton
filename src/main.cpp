#include <Arduino.h>
#include <LittleFS.h>
#include <SoftwareSerial.h>

#include <AQI.h>
#include <IotWebConf.h>

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

#define CONFIG_VERSION "v1"
#define PRODUCT_NAME "anton"
#define DEFAULT_PASSWORD "anton-system"

char sensor_name[40] = "anton-default";
char sensor_location[40] = "";
char influxdb_server[100] = "influxdb";
char influxdb_port[6] = "8086";
char influxdb_database[40] = "anton";
bool zh03b_enabled = true;
bool bme680_enabled = true;

char influxdb_url[200] = "";

bool configured = false;
bool should_save_config = false;

int wakeup_fail_counter = 0;
int connection_fail_counter = 0;
int last_measured = 0;
AirData last_values;
EnvironmentData last_env;
char last_status[10] = "NONE";
int last_aqi = 0;
char last_primary_contributor[5] = "NONE";

static const int NUM_SAMPLES = 10;
static const int MIN_SAMPLES_FOR_SUCCESS = 4;
static const int SENSOR_STARTUP_DELAY = 10000;
static const int SAMPLE_DELAY = 3000;
static const int MEASUREMENT_DELAY = 60000;
static const int MAX_JSON_DOCUMENT_SIZE = 2048;
static const int MAX_SENSOR_WAKE_FAIL = 5;
static const int MAX_CONNECTION_FAIL = 5;

// callback definitions
void wifiConnected();

DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;

IotWebConf iotWebConf(PRODUCT_NAME, &dnsServer, &server, DEFAULT_PASSWORD, CONFIG_VERSION);

IotWebConfParameter sensorNameParam = IotWebConfParameter(
    "Sensor Name",
    "barton",
    sensor_name,
    40);
IotWebConfParameter sensorLocationParam = IotWebConfParameter(
    "Location",
    "location",
    sensor_location,
    40);
IotWebConfParameter influxDbServerParam = IotWebConfParameter(
    "InfluxDB Server Host",
    "influxDbHost",
    influxdb_server,
    100);
IotWebConfParameter influxDbPortParam = IotWebConfParameter(
    "InfluxDB Port",
    "8086",
    influxdb_port,
    6);
IotWebConfParameter influxDbDatabaseParam = IotWebConfParameter(
    "InfluxDB Database",
    "anton",
    influxdb_database,
    40);

SoftwareSerial ZHSerial(D3, D4); // RX, TX
AirSensor *sensor = nullptr;
Reporter *reporter = nullptr;
EnvironmentSensor *environment;

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

void reset_all()
{
  WiFi.disconnect();
  LittleFS.format();
  ESP.restart();
}

void render_index_page(char *buf)
{
  int last = -1;
  if (last_measured > 0)
  {
    last = (millis() - last_measured) / 1000;
  }
  sprintf(
      buf,
      serverIndex,
      last,
      last_status,
      util::rnd(last_env.tempC),
      util::rnd(last_env.humPct),
      util::rnd(last_env.iaq),
      last_aqi,
      last_primary_contributor,
      millis() / 1000,
      sensor_name,
      influxdb_url,
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

  if (!configured)
  {
    server.sendHeader("Location", String("/config"), true);
    server.send(302, "text/plain", "");
    return;
  }

  char buf[2048];
  render_index_page(buf);

  server.send(200, "text/html", buf);
}

void setup()
{
  Serial.begin(9600);
  Serial.printf("setup: starting version %s\n", GIT_REV);

  iotWebConf.addParameter(&sensorNameParam);
  iotWebConf.addParameter(&sensorLocationParam);
  iotWebConf.addParameter(&influxDbServerParam);
  iotWebConf.addParameter(&influxDbPortParam);
  iotWebConf.addParameter(&influxDbDatabaseParam);

  iotWebConf.setWifiConnectionCallback(&wifiConnected);
  iotWebConf.getApTimeoutParameter()->visible = true;
  iotWebConf.setupUpdateServer(&httpUpdater);

  iotWebConf.init();

  server.on("/", handleRoot);

  server.on("/reset", HTTP_GET, []() {
    Serial.println("http: serving reset");
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", resetPage);
  });
  server.on("/reboot", HTTP_POST, []() {
    Serial.println("http: serving reboot");
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "rebooting");
    ESP.restart();
  });
  server.on("/reset-confirm", HTTP_POST, []() {
    Serial.println("http: serving reset confirm");
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "resetting");
    reset_all();
  });

  server.on("/config", [] { iotWebConf.handleConfig(); });
  server.onNotFound([]() { iotWebConf.handleNotFound(); });

  sprintf(influxdb_url, "http://%s:%s", influxdb_server, influxdb_port);
  Serial.printf("setup: influxdb url: %s\n", influxdb_url);

  // set up reporter
  InfluxDB_Reporter *DB = new InfluxDB_Reporter(sensor_name,
                                                sensor_location,
                                                influxdb_url,
                                                influxdb_database);
  reporter = DB;

  if (zh03b_enabled) {
    // set up ZH03B sensor
    ZHSerial.begin(9600);
    ZH03B_AirSensor *ZH = new ZH03B_AirSensor(ZHSerial);
    sensor = ZH;
  }

  if (bme680_enabled) {
    BME680_EnvironmentSensor *BME = new BME680_EnvironmentSensor(320, 150);
    environment = BME;
  }
}

void wifiConnected()
{
  configured = true;
}

int sort_uint16_desc(const void *cmp1, const void *cmp2)
{
  uint16_t a = *((uint16_t *)cmp1);
  uint16_t b = *((uint16_t *)cmp2);
  return b - a;
}

uint16_t median_value(uint16_t *values, int count)
{
  qsort(values, count, sizeof(values[0]), sort_uint16_desc);

  return values[(count / 2) - 1];
}

bool sample_sensor(AirData *sample)
{
  // wake sensor
  if (sensor->wake())
  {
    Serial.println("loop: sensor wakeup successfully");
    wakeup_fail_counter = 0;
  }
  else if (wakeup_fail_counter >= MAX_SENSOR_WAKE_FAIL)
  {
    Serial.printf("loop: ERROR failed to wake sensor %d times; resetting", wakeup_fail_counter);
    ESP.restart();
    return;
  }
  else
  {
    Serial.println("loop: ERROR failed to wake the sensor; delaying and trying again");
    wakeup_fail_counter += 1;
    _delay(30000);
    return;
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
    if (sensor->getAirData(&buf))
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
  if (sensor->sleep())
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

  sample->p1_0 = median_value(pm1_0, successes);
  sample->p2_5 = median_value(pm2_5, successes);
  sample->p10_0 = median_value(pm10_0, successes);

  return true;
}

void loop()
{
  if (!configured)
  {
    iotWebConf.doLoop();
    return;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("loop: wifi not connected; delaying");
    connection_fail_counter++;
    if (connection_fail_counter > 10)
    {
      Serial.println("loop: exceeded wifi connection failures, restarting");
      ESP.restart();
    }
    _delay(30000);
    return;
  }
  else
  {
    connection_fail_counter = 0;
  }

  EnvironmentData envSample;
  bool envSuccess = false;
  if (environment) {
    envSuccess = environment->getEnvironmentData(&envSample);
    if (envSuccess)
    {
      last_env = envSample;
    }
  }

  AirData sample;
  bool success = false;
  if (sensor) {
    success = sample_sensor(&sample);
    if (success)
    {
      Serial.printf("Aggregate: PM1.0, PM2.5, PM10=[%d %d %d]\n",
                    sample.p1_0,
                    sample.p2_5,
                    sample.p10_0);
      last_values = sample;
      last_measured = millis();
      strcpy(last_status, "SUCCESS");
    }
    else
    {
      Serial.println("loop: failed to sample sensor");
      strcpy(last_status, "FAILURE");
    }
  }

  CalculatedAQI aqi;
  bool aqi_success = false;

  if (success) {
    aqi_success = calculateAQI(sample, &aqi);
  }

  if (reporter->report(success ? &sample : nullptr, aqi_success ? &aqi : nullptr, envSuccess ? &envSample : nullptr))
  {
    Serial.println("loop: sample submitted successfully");
  }
  else
  {
    Serial.println("loop: failed to submit sample");
  }

  if (aqi_success)
  {
    last_aqi = aqi.value;
    strcpy(last_primary_contributor, aqi.pollutant);
  }
  else
  {
    last_aqi = -1;
    strcpy(last_primary_contributor, "NONE");
  }

  _delay(MEASUREMENT_DELAY);
}
