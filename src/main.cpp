#include <Arduino.h>

#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WiFiClient.h>

#include <SoftwareSerial.h>
#include <AQI.h>

#include "strings.h"
#include "sensors/AirSensor.h"
#include "sensors/ZH03B_AirSensor.h"
#include "reporters/Reporter.h"
#include "reporters/InfluxDB_Reporter.h"
#include "sensors/EnvironmentSensor.h"
#include "sensors/BME680_EnvironmentSensor.h"

#ifndef GIT_REV
#define GIT_REV "Unknown"
#endif

char sensor_name[40] = "anton-default";
char sensor_location[40] = "";
char influxdb_server[100] = "influxdb";
char influxdb_port[6] = "8086";
char influxdb_database[40] = "anton";

char influxdb_url[200] = "";

bool configured = false;
bool should_save_config = false;

int wakeup_fail_counter = 0;
int connection_fail_counter = 0;
int last_measured = 0;
AirData last_values;
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

SoftwareSerial ZHSerial(D3, D4); // RX, TX
AirSensor *sensor;
Reporter *reporter;
EnvironmentSensor *environment;

// web server; we only start this once configured, otherwise we rely on the
// configuration page that WiFiManager provides.
ESP8266WebServer server(80);

void save_config_callback()
{
  Serial.println("config: should save config");
  should_save_config = true;
}

void _delay(unsigned long ms)
{
  const int now = millis();

  while (1)
  {
    server.handleClient();
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
  SPIFFS.format();
  ESP.restart();
}

void read_config()
{
  Serial.println("config: mounting file system");

  if (SPIFFS.begin())
  {
    Serial.println("config: file system mounted");
    if (SPIFFS.exists("/config.json"))
    {
      Serial.println("config: file exists");

      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("config: opened file");

        DynamicJsonDocument doc(MAX_JSON_DOCUMENT_SIZE);

        deserializeJson(doc, configFile);

        if (!doc.isNull())
        {
          Serial.print("config: parsed json: ");
          serializeJson(doc, Serial);
          Serial.print("\n");

          strcpy(sensor_name, doc["sensor_name"]);
          strcpy(influxdb_server, doc["influxdb_server"]);
          strcpy(influxdb_port, doc["influxdb_port"]);
          strcpy(influxdb_database, doc["influxdb_database"]);

          // values added after the initial firmware, may be unset
          if (doc["sensor_location"])
          {
            strcpy(sensor_location, doc["sensor_location"]);
          }

          configured = true;
        }
        else
        {
          Serial.println("config: failed to load json");
        }

        configFile.close();
      }
    }
  }
  else
  {
    Serial.println("config: failed to mount file system");
  }
}

void save_config()
{
  Serial.println("config: saving config");

  DynamicJsonDocument doc(MAX_JSON_DOCUMENT_SIZE);

  doc["sensor_name"] = sensor_name;
  doc["sensor_location"] = sensor_location;
  doc["influxdb_server"] = influxdb_server;
  doc["influxdb_port"] = influxdb_port;
  doc["influxdb_database"] = influxdb_database;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("config: failed to open file for writing");
    return;
  }

  serializeJson(doc, configFile);
  serializeJson(doc, Serial);

  configFile.close();
  configured = true;
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
      last_values.p1_0,
      last_values.p2_5,
      last_values.p10_0,
      last_aqi,
      last_primary_contributor,
      millis() / 1000,
      sensor_name,
      influxdb_url,
      GIT_REV);
}

void render_config_page(char *buf)
{
  sprintf(
      buf,
      configPage,
      sensor_name,
      sensor_location,
      influxdb_server,
      influxdb_port,
      influxdb_database);
}

void run_http_server()
{
  server.on("/", HTTP_GET, []() {
    Serial.println("http: serving index");
    server.sendHeader("Connection", "close");
    char buf[1024] = "";
    render_index_page(buf);
    server.send(200, "text/html", buf);
  });

  server.on("/config", HTTP_GET, []() {
    Serial.println("http: serving config");
    server.sendHeader("Connection", "close");
    char buf[2048] = "";
    render_config_page(buf);
    server.send(200, "text/html", buf);
  });

  server.on("/config", HTTP_POST, []() {
    Serial.println("http: saving config");

    if (server.hasArg("sensor_name"))
    {
      strcpy(sensor_name, server.arg("sensor_name").c_str());
    }
    if (server.hasArg("sensor_location"))
    {
      strcpy(sensor_location, server.arg("sensor_location").c_str());
    }
    if (server.hasArg("influxdb_server"))
    {
      strcpy(influxdb_server, server.arg("influxdb_server").c_str());
    }
    if (server.hasArg("influxdb_port"))
    {
      strcpy(influxdb_port, server.arg("influxdb_port").c_str());
    }
    if (server.hasArg("influxdb_database"))
    {
      strcpy(influxdb_database, server.arg("influxdb_database").c_str());
    }

    save_config();
    Serial.println("http: config saved, restarting");

    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "saved, restarting");
    delay(100);
    ESP.restart();
  });

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

  server.on(
      "/update",
      HTTP_POST,
      []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
      },
      []() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          Serial.setDebugOutput(true);
          WiFiUDP::stopAll();
          Serial.printf("Update: %s\n", upload.filename.c_str());
          uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
          if (!Update.begin(maxSketchSpace))
          { //start with max available size
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (Update.end(true))
          { //true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          }
          else
          {
            Update.printError(Serial);
          }
          Serial.setDebugOutput(false);
        }
        yield();
      });

  Serial.println("http: running server");
  server.begin();
}

void setup()
{
  Serial.begin(9600);
  Serial.printf("setup: starting version %s\n", GIT_REV);
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(save_config_callback);

  read_config();

  WiFiManagerParameter sensor_name_param("sensor_name", "sensor name", sensor_name, 40);
  wifiManager.addParameter(&sensor_name_param);
  WiFiManagerParameter sensor_location_param("sensor_location", "sensor location", sensor_location, 40);
  wifiManager.addParameter(&sensor_location_param);
  WiFiManagerParameter influxdb_server_param("influxdb_server", "influxdb server", influxdb_server, 100);
  wifiManager.addParameter(&influxdb_server_param);
  WiFiManagerParameter influxdb_port_param("influxdb_port", "influxdb port", influxdb_port, 6);
  wifiManager.addParameter(&influxdb_port_param);
  WiFiManagerParameter influxdb_database_param("influxdb_database", "influxdb database", influxdb_database, 40);
  wifiManager.addParameter(&influxdb_database_param);

  wifiManager.autoConnect("anton-setup");
  Serial.println("setup: wifi connected");

  //read updated parameters
  strcpy(sensor_name, sensor_name_param.getValue());
  strcpy(sensor_location, sensor_location_param.getValue());
  strcpy(influxdb_server, influxdb_server_param.getValue());
  strcpy(influxdb_port, influxdb_port_param.getValue());
  strcpy(influxdb_database, influxdb_database_param.getValue());

  // create influxdb url
  sprintf(influxdb_url, "http://%s:%s", influxdb_server, influxdb_port);
  Serial.printf("setup: influxdb url: %s\n", influxdb_url);

  if (should_save_config)
  {
    save_config();
  }

  // start http server
  run_http_server();

  // set up reporter
  InfluxDB_Reporter *DB = new InfluxDB_Reporter(sensor_name,
                                                sensor_location,
                                                influxdb_url,
                                                influxdb_database);
  reporter = DB;

  // set up ZH03B sensor
  ZHSerial.begin(9600);
  ZH03B_AirSensor *ZH = new ZH03B_AirSensor(ZHSerial);
  sensor = ZH;

  BME680_EnvironmentSensor *BME = new BME680_EnvironmentSensor();
  environment = BME;
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
      if (buf.p1_0 == buf.p2_5 == buf.p10_0 && buf.p1_0 > 500)
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

float altitude(const int32_t press, const float seaLevel = 1013.25);
float altitude(const int32_t press, const float seaLevel)
{
  /*!
  @brief     This converts a pressure measurement into a height in meters
  @details   The corrected sea-level pressure can be passed into the function if it is known,
             otherwise the standard atmospheric pressure of 1013.25hPa is used (see
             https://en.wikipedia.org/wiki/Atmospheric_pressure) for details.
  @param[in] press    Pressure reading from BME680
  @param[in] seaLevel Sea-Level pressure in millibars
  @return    floating point altitude in meters.
  */
  static float Altitude;
  Altitude =
      44330.0 * (1.0 - pow(((float)press / 100.0) / seaLevel, 0.1903)); // Convert into meters
  return (Altitude);
} // of method altitude()

void loop()
{
  if (!configured)
  {
    Serial.println("loop: unconfigured, resetting");
    reset_all();
    delay(5000);
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

  EnvironmentData envSample;
  bool envSuccess = environment->getEnvironmentData(&envSample);

  AirData sample;
  bool success = sample_sensor(&sample);
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

  // shut down sensor, stopping its fan.
  if (sensor->sleep())
  {
    Serial.println("loop: sensor sleep successful");
  }
  else
  {
    Serial.println("loop: ERROR failed to sleep the sensor");
  }

  if (success && false)
  {
    CalculatedAQI aqi;
    bool aqi_success = calculateAQI(sample, &aqi);
    if (reporter->report(&sample, aqi_success ? &aqi : nullptr, envSuccess ? &envSample : nullptr))
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
  }

  _delay(5000);
}
