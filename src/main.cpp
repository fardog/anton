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
static int NUM_ATTEMPTS = 5;
static int SAMPLE_DELAY = 5000;
static int MEASUREMENT_DELAY = 60000;
static char* URL_TEMPLATE = "http://%s:%s/write?db=%s";
static char* MEASUREMENT_TEMPLATE = "particulate_matter,node=%s p1_0=%d,p2_5=%d,p10_0=%d";

SoftwareSerial ZHSerial(D1, D2); // RX, TX
SD_ZH03B ZH03B( ZHSerial );

void save_config_callback () {
  Serial.println("Should save config");
  should_save_config = true;
}

void reset_all() {
  WiFi.disconnect();
  SPIFFS.format();
}

void read_config() {
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, configFile);
        // print to output
        serializeJson(doc, Serial);
        if (!doc.isNull()) {
          Serial.println("\nparsed json");

          strcpy(sensor_name, doc["sensor_name"]);
          strcpy(influxdb_server, doc["influxdb_server"]);
          strcpy(influxdb_port, doc["influxdb_port"]);
          strcpy(influxdb_database, doc["influxdb_database"]);
          configured = true;

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
}

void save_config() {
  Serial.println("saving config");

  DynamicJsonDocument doc(2048);

  doc["sensor_name"] = sensor_name;
  doc["influxdb_server"] = influxdb_server;
  doc["influxdb_port"] = influxdb_port;
  doc["influxdb_database"] = influxdb_database;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
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
  Serial.println("Connected!");

  //read updated parameters
  strcpy(sensor_name, sensor_name_param.getValue());
  strcpy(influxdb_server, influxdb_server_param.getValue());
  strcpy(influxdb_port, influxdb_port_param.getValue());
  strcpy(influxdb_database, influxdb_database_param.getValue());

  // create influxdb url
  sprintf(influxdb_url, URL_TEMPLATE, influxdb_server, influxdb_port, influxdb_database);
  Serial.print("Configured for influxdb url: ");
  Serial.println(influxdb_url);

  if (should_save_config) {
    save_config();
  }

  ZHSerial.begin(9600);
  delay(100);
  ZH03B.setMode( SD_ZH03B::IU_MODE );
  Serial.println("-- Reading ZH03B --");
  delay(200);

  if (ZH03B.sleep())
    Serial.println("Sleep mode confirmed");
}

bool read_sensor_data(uint16_t* arr) {
  Serial.println("sampling");
  if( ZH03B.readData() ) {
    // char printbuf1[80];
    // Serial.print( ZH03B.getMode() == SD_ZH03B::IU_MODE ? "IU:" : "Q&A:" );  
    // sprintf(printbuf1, "PM1.0, PM2.5, PM10=[%d %d %d]", ZH03B.getPM1_0(), ZH03B.getPM2_5(), ZH03B.getPM10_0() );
    // Serial.println(printbuf1);

    arr[0] = ZH03B.getPM1_0();
    arr[1] = ZH03B.getPM2_5();
    arr[2] = ZH03B.getPM10_0();

    return true;
  } else {   
    Serial.println( "ZH03B Error reading stream or Check Sum Error" );
    return false;
  } 
}

bool sample_sensor(int* arr) {
  Serial.println("attemping average sample");
  uint16_t pm1_0[NUM_ATTEMPTS];
  uint16_t pm2_5[NUM_ATTEMPTS];
  uint16_t pm10_0[NUM_ATTEMPTS];
  int successes = 0;

  uint16_t buf[3];

  for (int i = 0; i < NUM_ATTEMPTS; i++) {
    if (read_sensor_data(buf)) {
      if (buf[0] == buf[1] == buf[2] && buf[0] > 500) {
        Serial.println("faulty measurement, all values are equal and very large. discarding");
      } else {
        pm1_0[successes] = buf[0];
        pm2_5[successes] = buf[1];
        pm10_0[successes] = buf[2];
        successes++;

        char printbuf[80];
        sprintf(printbuf, "Sample: PM1.0, PM2.5, PM10=[%d %d %d]", buf[0], buf[1], buf[2]);
        Serial.println(printbuf);
      }
    }
    delay(SAMPLE_DELAY);
  }

  if (successes == 0) {
    return false;
  }

  int pm1_0_agg = 0;
  int pm2_5_agg = 0;
  int pm10_0_agg = 0;
  for (int i = 0; i < successes; i++) {
    pm1_0_agg += pm1_0[i];
    pm2_5_agg += pm2_5[i];
    pm10_0_agg += pm10_0[i];
  }

  arr[0] = pm1_0_agg / successes;
  arr[1] = pm2_5_agg / successes;
  arr[2] = pm10_0_agg / successes;

  return true;
}

bool post_measurement(int* values) {
  bool success = false;
  HTTPClient http;
  http.begin(influxdb_url);

  char measurement[100];
  sprintf(measurement, MEASUREMENT_TEMPLATE, sensor_name, values[0], values[1], values[2]);
  Serial.println(measurement);

  int httpCode = http.POST(measurement);
  if (httpCode == HTTP_CODE_NO_CONTENT) {
    success = true;
  } else {
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);
  }

  http.end();

  return success;
}

void loop()
{
  if (!configured) {
    Serial.println("unconfigured, resetting");
    reset_all();
    delay(5000);
    return;
  }

  if (ZH03B.wakeup())
    Serial.println("Woke up successfully");

  int sample[3];
  if (sample_sensor(sample)) {
    char printbuf[80];
    sprintf(printbuf, "Aggregate: PM1.0, PM2.5, PM10=[%d %d %d]", sample[0], sample[1], sample[2]);
    Serial.println(printbuf);
  } else {
    Serial.println("failed to sample sensor");
  }

  if (ZH03B.sleep())
    Serial.println("Sleep mode confirmed");

  if (post_measurement(sample)) {
    Serial.println("successfully submitted sample");
  } else {
    Serial.println("failed to submit sample");
  }

  delay(MEASUREMENT_DELAY);
}
