#include "InfluxDB_Reporter.h"

InfluxDB_Reporter::InfluxDB_Reporter(const char *sensorName,
                                     const char *sensorLocation,
                                     const char *serverUrl,
                                     const char *db,
                                     bool debug)
    : _client(serverUrl, db),
      _sensorName(sensorName),
      _sensorLocation(sensorLocation),
      _debug(debug)
{
}

bool InfluxDB_Reporter::report(AirData *air, CalculatedAQI *aqi, EnvironmentData *env)
{
    Point measurement("particulate_matter");
    measurement.addTag("node", _sensorName);
    if (strcmp(_sensorLocation, ""))
    {
        measurement.addTag("location", _sensorLocation);
    }

    measurement.addField("p1_0", (int)air->p1_0);
    measurement.addField("p2_5", (int)air->p2_5);
    measurement.addField("p10_0", (int)air->p10_0);

    if (aqi)
    {
        measurement.addField("aqi", (int)round(aqi->value));
        measurement.addField("aqi_contributor", aqi->pollutant);
    }

    if (env)
    {
        measurement.addField("temperature", env->tempC);
        measurement.addField("humidity", env->humPct);
        measurement.addField("pressure", env->pressure);
        measurement.addField("gas_resistance", env->gasResistance);
    }

    Serial.print("post_measurement: ");
    Serial.println(measurement.toLineProtocol());

    return _client.writePoint(measurement);
}

String InfluxDB_Reporter::getLastErrorMessage()
{
    return _client.getLastErrorMessage();
}
