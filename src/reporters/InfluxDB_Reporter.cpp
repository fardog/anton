#include "InfluxDB_Reporter.h"
#include "util.h"

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

bool InfluxDB_Reporter::report(AirData *air, CalculatedAQI *aqi, EnvironmentData *env, CO2Data *co2)
{
  Point measurement("particulate_matter");
  measurement.addTag("node", _sensorName);
  if (strcmp(_sensorLocation, ""))
  {
    measurement.addTag("location", _sensorLocation);
  }

  if (air)
  {
    measurement.addField("p1_0", (int)air->p1_0);
    measurement.addField("p2_5", (int)air->p2_5);
    measurement.addField("p10_0", (int)air->p10_0);
  }

  if (aqi)
  {
    measurement.addField("aqi", util::rnd(aqi->value));
    measurement.addField("aqi_contributor", aqi->pollutant);
  }

  if (env)
  {
    measurement.addField("temperature", util::rnd(env->tempC));
    measurement.addField("humidity", util::rnd(env->humPct));
    measurement.addField("pressure", util::rnd(env->pressure));
    measurement.addField("gas_resistance", util::rnd(env->gasResistance));
    measurement.addField("iaq", util::rnd(env->iaq));
  }

  if (co2)
  {
    measurement.addField("co2", (int)co2->ppm);
  }

  Serial.print("post_measurement: ");
  Serial.println(measurement.toLineProtocol());

  return _client.writePoint(measurement);
}

String InfluxDB_Reporter::getLastErrorMessage()
{
  return _client.getLastErrorMessage();
}
