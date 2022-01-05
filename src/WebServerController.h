#ifndef WebServerController_h
#define WebServerController_h

#include <IotWebConf.h>

#include "interfaces/Looper.h"
#include "NetworkController.h"

#ifdef ESP8266
#define ANTON_PLATFORM "ESP8266"
#elif defined(ESP32)
#define ANTON_PLATFORM "ESP32"
#endif

class WebServerController : public Looper
{
public:
  WebServerController(WebServer *webServer, NetworkController *networkController, SensorController *SensorController);
  void loop(){};

private:
  void _handleIndexPage();
  void _handleHardReset();
  void _handleResetAll();
  void _handleGetCalibrate();
  void _handlePostCalibrate();
  void _renderIndexPage(char *buf);

  WebServer *_webServer;
  NetworkController *_networkController;
  SensorController *_sensorController;
};

#endif
