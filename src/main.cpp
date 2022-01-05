#include <Arduino.h>

#include <AQI.h>

#include <IotWebConf.h>
#include <IotWebConfTParameter.h>

#include "strings.h"

#include "NetworkController.h"
#include "SensorController.h"
#include "WebServerController.h"
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

#ifdef ESP8266
#define ANTON_PLATFORM "ESP8266"
#elif defined(ESP32)
#define ANTON_PLATFORM "ESP32"
#endif

#define PRODUCT_NAME "anton"
#define DEFAULT_PASSWORD "anton-system"

#ifndef BSEC_TEMPERATURE_OFFSET_C
#define BSEC_TEMPERATURE_OFFSET_C 4.0
#endif

// pins
#ifdef ESP8266
#define PARTICULATE_RX D3
#define PARTICULATE_TX D4
// TODO: reevaluate these
#define CO2_RX D5
#define CO2_TX D6
#elif defined(ESP32)
#define PARTICULATE_UART 2
#define CO2_UART 1
#define UART_1_RX 18
#define UART_1_TX 19
#endif

DNSServer dnsServer;
WebServer webServer(80);

Stream *airSensorSerial = nullptr;
AirSensor *airSensor = nullptr;
Reporter *reporter = nullptr;
EnvironmentSensor *environmentSensor = nullptr;
Stream *co2SensorSerial = nullptr;
CO2Sensor *co2Sensor = nullptr;

SensorController *sensorController = nullptr;
NetworkController *networkController = nullptr;
WebServerController *webserverController = nullptr;

AntonConfiguration config;

void setup()
{
  Serial.begin(115200);
  Serial.printf("setup: starting version %s\n", GIT_REV);

  networkController = new NetworkController(PRODUCT_NAME, DEFAULT_PASSWORD, &dnsServer, &webServer);

  config = networkController->getConfig();

  // set up reporter
  InfluxDB_Reporter *DB = new InfluxDB_Reporter(config.sensorName,
                                                config.sensorLocation,
                                                config.influxdbUrl,
                                                config.influxdbDatabase);
  reporter = DB;

  if (config.particleSensorEnabled)
  {
#ifdef ESP8266
    airSensorSerial = util::getSerial(PARTICULATE_RX, PARTICULATE_TX);
#elif defined(ESP32)
    airSensorSerial = util::getSerial(PARTICULATE_UART);
#endif

    if (config.particleSensor == ParticleSensor::Winsen_ZH03B)
    {
      Serial.println("setup: starting ZH03B particle sensor on serial");
      ZH03B_AirSensor *ZH = new ZH03B_AirSensor(*airSensorSerial);
      airSensor = ZH;
    }
    else if (config.particleSensor == ParticleSensor::Plantower_PMS7003)
    {
      Serial.println("setup: starting PMS7003 particle sensor on serial");
      PMS_AirSensor *PMS = new PMS_AirSensor(*airSensorSerial);
      airSensor = PMS;
    }
    else
    {
      Serial.printf("setup: invalid particle sensor type: %d\n", config.particleSensor);
    }
  }

  if (config.vocSensorEnabled)
  {
    Serial.println("setup: starting BME680 VOC sensor on I2C (default pins)");
    Wire.begin();
    BME680_EnvironmentSensor *BME = new BME680_EnvironmentSensor(Wire, BSEC_TEMPERATURE_OFFSET_C);
    environmentSensor = BME;
  }

  if (config.co2SensorEnabled)
  {
#ifdef ESP8266
    co2SensorSerial = util::getSerial(CO2_RX, CO2_TX);
#elif defined(ESP32)
    co2SensorSerial = util::getSerial(CO2_UART, 9600, SERIAL_8N1, UART_1_RX, UART_1_TX);
#endif
    co2Sensor = new MHZ19B_CO2Sensor(co2SensorSerial);
  }

  sensorController = new SensorController(reporter, airSensor, co2Sensor, environmentSensor);
  webserverController = new WebServerController(&webServer, networkController, sensorController);
}

void loop()
{
  networkController->loop();
  webserverController->loop();
  sensorController->loop();
}
