#ifndef strings_h
#define strings_h

const char *serverIndex = R"(
<!DOCTYPE html>
<html>
  <body>
    <h1>Measurements</h1>
    Measured %d seconds ago; last result was %s.
    <dl>
      <dt>PM1.0</dt>
      <dd>%d</dd>

      <dt>PM2.5</dt>
      <dd>%d</dd>

      <dt>PM10</dt>
      <dd>%d</dd>

      <dt>AQI</dt>
      <dd>%d</dd>

      <dt>Primary Contributor</dt>
      <dd>%s</dd>
    </dl>
    <h1>Stats</h1>
    Uptime: %d seconds
    <h1>Configuration</h1>
    <dl>
      <dt>Sensor Name</dt>
      <dd>%s</dd>

      <dt>InfluxDB URL</dt>
      <dd>%s</dd>
    </dl>
    Visit the <a href="/config">config page</a>.
    <h1>Update</h1>
    <form method="POST" action="/update" enctype="multipart/form-data">
      <input type="file" name="update">
      <input type="submit" value="Update">
    </form>
    <h1>Reset</h1>
    Visit the <a href="/reset">reset page</a>.
  </body>
</html>
)";
const char *resetPage = R"(
<!DOCTYPE html>
<html>
  <body>
    <a href="/">&lt; Back</a>
    <h1>Reboot</h1>
    This will reboot the sensor.
    <form method="POST" action="/reboot">
      <input type="submit" value="Reboot">
    </form>
    <h1>Reset All</h1>
    This will reset the sensor to its default values, and put it into setup mode.
    <form method="POST" action="/reset-confirm">
      <input type="submit" value="Reset All">
    </form>
  </body>
</html>
)";
const char *configPage = R"(
<!DOCTYPE html>
<html>
  <body>
    <a href="/">&lt; Back</a>
    <h1>Configuration</h1>
    <form method="POST" action="/config" enctype="multipart/form-data">
      <h2>Sensor</h2>
      <div>
        <label>
          Name
          <input type="text" name="sensor_name" value="%s" placeholder="sensor name">
        </label>
      </div>
      <div>
        <label>
          Location
          <input type="text" name="sensor_location" value="%s" placeholder="sensor location">
        </label>
      </div>
      <h2>InfluxDB Server</h2>
      <div>
        <label>
          Hostname
          <input type="text" name="influxdb_server" value="%s" placeholder="influxdb hostname">
        </label>
      </div>
      <div>
        <label>
          Port
          <input type="text" name="influxdb_port" value="%s" placeholder="influxdb port">
        </label>
      </div>
      <div>
        <label>
          Database
          <input type="text" name="influxdb_database" value="%s" placeholder="influxdb database">
        </label>
      </div>
      <input type="submit" value="Save">
    </form>
  </body>
</html>
)";

#endif
