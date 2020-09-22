#include <Arduino.h>

#include <ArduinoJson.h>

#include <ESP8266HTTPClient.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <SD_ZH03B.h>
#include <SoftwareSerial.h>
#include <AQI.h>

#include "strings.h"

char sensor_name[40] = "anton-default";
char influxdb_server[100] = "influxdb";
char influxdb_port[6] = "8086";
char influxdb_database[40] = "anton";

char influxdb_url[200] = "";

bool configured = false;
bool should_save_config = false;

int wakeup_fail_counter = 0;
int last_measured = 0;
int last_values[3] = {0, 0, 0};
char last_status[10] = "NONE";

static const int NUM_SAMPLES = 10;
static const int MIN_SAMPLES_FOR_SUCCESS = 4;
static const int SENSOR_STARTUP_DELAY = 10000;
static const int SAMPLE_DELAY = 3000;
static const int MEASUREMENT_DELAY = 60000;
static const int MAX_JSON_DOCUMENT_SIZE = 2048;
static const int MAX_SENSOR_WAKE_FAIL = 5;
static const char *URL_TEMPLATE = "http://%s:%s/write?db=%s";
static const char *MEASUREMENT_TEMPLATE_AQI = "particulate_matter,node=%s p1_0=%d,p2_5=%d,p10_0=%d,aqi=%d,aqi_contributor=%s";
static const char *MEASUREMENT_TEMPLATE = "particulate_matter,node=%s p1_0=%d,p2_5=%d,p10_0=%d";

SoftwareSerial ZHSerial(D1, D2); // RX, TX
SD_ZH03B ZH03B(ZHSerial);

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
      last_values[0],
      last_values[1],
      last_values[2],
      millis() / 1000,
      sensor_name,
      influxdb_url);
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
  WiFiManager wifiManager;
  // reset_all();
  wifiManager.setSaveConfigCallback(save_config_callback);

  read_config();

  WiFiManagerParameter sensor_name_param("sensor_name", "sensor name", sensor_name, 40);
  wifiManager.addParameter(&sensor_name_param);
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
  strcpy(influxdb_server, influxdb_server_param.getValue());
  strcpy(influxdb_port, influxdb_port_param.getValue());
  strcpy(influxdb_database, influxdb_database_param.getValue());

  // create influxdb url
  sprintf(influxdb_url, URL_TEMPLATE, influxdb_server, influxdb_port, influxdb_database);
  Serial.printf("setup: influxdb url: %s\n", influxdb_url);

  if (should_save_config)
  {
    save_config();
  }

  // start http server
  run_http_server();

  // set up ZH03B sensor
  ZHSerial.begin(9600);
  delay(100);
  ZH03B.setMode(SD_ZH03B::IU_MODE);
  delay(200);

  if (ZH03B.sleep())
  {
    Serial.println("setup: sleep confirmed");
  }
}

bool read_sensor_data(uint16_t *arr)
{
  Serial.println("sensor: sampling");
  if (ZH03B.readData())
  {
    arr[0] = ZH03B.getPM1_0();
    arr[1] = ZH03B.getPM2_5();
    arr[2] = ZH03B.getPM10_0();

    return true;
  }
  else
  {
    Serial.println("sensor: Error reading stream or Check Sum Error");
    return false;
  }
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

bool sample_sensor(int *arr)
{
  Serial.println("sensor: attemping average sample");
  uint16_t pm1_0[NUM_SAMPLES];
  uint16_t pm2_5[NUM_SAMPLES];
  uint16_t pm10_0[NUM_SAMPLES];
  int successes = 0;

  uint16_t buf[3];

  for (int i = 0; i < NUM_SAMPLES; i++)
  {
    if (read_sensor_data(buf))
    {
      if (buf[0] == buf[1] == buf[2] && buf[0] > 500)
      {
        Serial.println("sensor: faulty measurement, all values are equal and very large. discarding");
      }
      else
      {
        pm1_0[successes] = buf[0];
        pm2_5[successes] = buf[1];
        pm10_0[successes] = buf[2];
        successes++;

        Serial.printf("sensor: sample PM1.0, PM2.5, PM10=[%d, %d, %d]\n", buf[0], buf[1], buf[2]);
      }
    }
    _delay(SAMPLE_DELAY);
  }

  if (successes < MIN_SAMPLES_FOR_SUCCESS)
  {
    Serial.println("sensor: failed to get minimum number of samples");
    return false;
  }

  arr[0] = median_value(pm1_0, successes);
  arr[1] = median_value(pm2_5, successes);
  arr[2] = median_value(pm10_0, successes);

  return true;
}

struct CalculatedAQI
{
  float value;
  char *pollutant;
};

bool calculate_aqi(int *values, CalculatedAQI *aqi)
{
  AQI::Measurement list[2] = {
      AQI::Measurement(AQI::PM2_5, values[1]),
      AQI::Measurement(AQI::PM10, values[2])};

  AQI::Measurements measurements = AQI::Measurements(list, 2);

  aqi->value = measurements.getAQI();
  if (aqi->value == -1)
  {
    return false;
  }

  AQI::Pollutant pollutant = measurements.getPollutant();
  switch (pollutant)
  {
  case AQI::PM2_5:
    strcpy(aqi->pollutant, "p2_5");
    break;
  case AQI::PM10:
    strcpy(aqi->pollutant, "p10_0");
    break;
  default:
    return false;
  }

  return true;
}

void format_measurement(char *buf, int *values, CalculatedAQI *aqi)
{
  if (aqi)
  {
    int aqi_value = round(aqi->value);
    sprintf(buf, MEASUREMENT_TEMPLATE_AQI, sensor_name, values[0], values[1], values[2], aqi_value, aqi->pollutant);
  }
  else
  {
    sprintf(buf, MEASUREMENT_TEMPLATE, sensor_name, values[0], values[1], values[2]);
  }
}

bool post_measurement(int *values, CalculatedAQI *aqi)
{
  char measurement[120];
  format_measurement(measurement, values, aqi);
  Serial.printf("influxdb: %s\n", measurement);

  bool success = false;
  HTTPClient http;
  http.begin(influxdb_url);

  int httpCode = http.POST(measurement);
  if (httpCode == HTTP_CODE_NO_CONTENT)
  {
    success = true;
  }
  else
  {
    Serial.printf("influxdb: failed with status: %d\n", httpCode);
  }

  http.end();

  return success;
}

void loop()
{
  if (!configured)
  {
    Serial.println("loop: unconfigured, resetting");
    reset_all();
    delay(5000);
    return;
  }

  // wake sensor
  if (ZH03B.wakeup())
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

  int sample[3];
  bool success = sample_sensor(sample);
  if (success)
  {
    Serial.printf("Aggregate: PM1.0, PM2.5, PM10=[%d %d %d]\n", sample[0], sample[1], sample[2]);
    for (int i = 0; i < 3; i++)
    {
      last_values[i] = sample[i];
    }
    last_measured = millis();
    strcpy(last_status, "SUCCESS");
  }
  else
  {
    Serial.println("loop: failed to sample sensor");
    strcpy(last_status, "FAILURE");
  }

  // shut down sensor, stopping its fan.
  if (ZH03B.sleep())
  {
    Serial.println("loop: sensor sleep successful");
  }
  else
  {
    Serial.println("loop: ERROR failed to sleep the sensor");
  }

  if (success)
  {
    char pollutant[5];
    CalculatedAQI aqi = {-1, pollutant};

    if (post_measurement(sample, calculate_aqi(sample, &aqi) ? &aqi : nullptr))
    {
      Serial.println("loop: sample submitted successfully");
    }
    else
    {
      Serial.println("loop: failed to submit sample");
    }
  }

  _delay(MEASUREMENT_DELAY);
}
