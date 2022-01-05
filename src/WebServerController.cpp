#include "WebServerController.h"
#include "strings.h"
#include "util.h"

WebServerController::WebServerController(WebServer *webServer, NetworkController *networkController, SensorController *sensorController)
    : _webServer(webServer),
      _networkController(networkController),
      _sensorController(sensorController)
{
  _webServer->on("/", [this]()
                 { _handleIndexPage(); });

  _webServer->on("/config", [this]()
                 { _networkController->handleConfig(); });
  _webServer->onNotFound([this]()
                         { _networkController->handleNotFound(); });

  _webServer->on("/calibrate", [this]()
                 { _handleGetCalibrate(); });
  _webServer->on("/calibrate-confirm", HTTP_POST, [this]()
                 { _handlePostCalibrate(); });
  _webServer->on("/reset", HTTP_GET, [this]()
                 {
                   Serial.println("http: serving reset");
                   _webServer->sendHeader("Connection", "close");
                   _webServer->send(200, "text/html", resetPage);
                 });
  _webServer->on("/reboot", HTTP_POST, [this]()
                 {
                   Serial.println("http: serving reboot");
                   _webServer->sendHeader("Connection", "close");
                   _webServer->send(200, "text/plain", "rebooting");
                   ESP.restart();
                 });
  _webServer->on("/reset-confirm", HTTP_POST, [this]()
                 {
                   Serial.println("http: serving reset confirm");
                   _webServer->sendHeader("Connection", "close");
                   _webServer->send(200, "text/plain", "resetting");
                   _handleResetAll();
                 });
  _webServer->on("/hard-reset-confirm", HTTP_POST, [this]()
                 {
                   Serial.println("http: serving hard reset confirm");
                   _webServer->sendHeader("Connection", "close");
                   _webServer->send(200, "text/plain", "resetting");
                   _handleHardReset();
                 });
}

void WebServerController::_handleIndexPage()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (_networkController->hasHandledCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }

  if (_networkController)
  {
    char buf[2048];
    _renderIndexPage(buf);
    _webServer->send(200, "text/html", buf);
  }
  else
  {
    _webServer->send(200, "text/html", serverUnconfigured);
  }
}

void WebServerController::_handleHardReset()
{
  _networkController->resetConfig(true);
}

void WebServerController::_handleResetAll()
{
  _networkController->resetConfig();
}

void WebServerController::_handleGetCalibrate()
{
  _webServer->send(200, "text/html", calibrationPage);
}

void WebServerController::_handlePostCalibrate()
{
  String co2Success = String("NOT PRESENT");
  AntonConfiguration config = _networkController->getConfig();
  if (config.co2SensorEnabled)
  {
    if (_sensorController->calibrate())
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
  _webServer->send(200, "text/html", buf);
}

void WebServerController::_renderIndexPage(char *buf)
{
  AntonConfiguration config = _networkController->getConfig();
  AirData ad = _sensorController->airData();
  EnvironmentData ed = _sensorController->environmentData();
  CalculatedAQI aqi = _sensorController->aqi();
  CO2Data co2 = _sensorController->co2Data();

  int lastMeasured = -1;
  if (ad.timestamp > 0)
  {
    lastMeasured = (millis() - ad.timestamp) / 1000;
  }

  int reported = -1;
  if (_sensorController->lastReported() > 0)
  {
    reported = (millis() - _sensorController->lastReported()) / 1000;
  }

  String lastStatus = _sensorController->lastErrorMessage();
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
      config.sensorName,
      config.influxdbUrl,
      GIT_REV,
      ANTON_PLATFORM);
}
