#include <Arduino.h>

#include <ArduinoJson.h>

#include <ESP8266HTTPClient.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <SD_ZH03B.h>
#include <SoftwareSerial.h>

char sensor_name[40] = "anton-default";
char influxdb_server[100] = "influxdb";
char influxdb_port[6] = "8086";
char influxdb_database[40] = "anton";

char influxdb_url[200] = "";

bool configured = false;
bool should_save_config = false;

static const int NUM_SAMPLES = 10;
static const int MIN_SAMPLES_FOR_SUCCESS = 4;
static const int SAMPLE_DELAY = 3000;
static const int MEASUREMENT_DELAY = 30000;
static const int MAX_JSON_DOCUMENT_SIZE = 2048;
static const char *URL_TEMPLATE = "http://%s:%s/write?db=%s";
static const char *MEASUREMENT_TEMPLATE = "particulate_matter,node=%s p1_0=%d,p2_5=%d,p10_0=%d";

SoftwareSerial ZHSerial(D1, D2); // RX, TX
SD_ZH03B ZH03B(ZHSerial);

void save_config_callback()
{
  Serial.println("config: should save config");
  should_save_config = true;
}

void reset_all()
{
  WiFi.disconnect();
  SPIFFS.format();
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
    delay(SAMPLE_DELAY);
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

bool post_measurement(int *values)
{
  bool success = false;
  HTTPClient http;
  http.begin(influxdb_url);

  char measurement[100];
  sprintf(measurement, MEASUREMENT_TEMPLATE, sensor_name, values[0], values[1], values[2]);
  Serial.printf("influxdb: %s\n", measurement);

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

  if (ZH03B.wakeup())
  {
    Serial.println("loop: sensor wakeup successfully");
  }

  int sample[3];
  bool success = sample_sensor(sample);
  if (success)
  {
    Serial.printf("Aggregate: PM1.0, PM2.5, PM10=[%d %d %d]\n", sample[0], sample[1], sample[2]);
  }
  else
  {
    Serial.println("loop: failed to sample sensor");
  }

  if (ZH03B.sleep())
    Serial.println("loop: sensor sleep successful");

  if (success)
  {
    if (post_measurement(sample))
    {
      Serial.println("loop: sample submitted successfully");
    }
    else
    {
      Serial.println("loop: failed to submit sample");
    }
  }

  delay(MEASUREMENT_DELAY);
}
