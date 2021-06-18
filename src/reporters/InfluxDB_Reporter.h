#ifndef InfluxDB_Reporter_h
#define InfluxDB_Reporter_h

#include <InfluxDbClient.h>

#include "Reporter.h"
#include "sensors/AirSensor.h"

class InfluxDB_Reporter : public Reporter
{
public:
    InfluxDB_Reporter(const char *sensorName,
                      const char *sensorLocation,
                      const char *serverUrl,
                      const char *db,
                      bool debug = false);
    bool report(AirData *air, CalculatedAQI *aqi, EnvironmentData *env);
    String getLastErrorMessage();

private:
    InfluxDBClient _client;
    const char *_sensorName;
    const char *_sensorLocation;
    bool _debug;
};

#endif
