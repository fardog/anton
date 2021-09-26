#include "strings.h"

const char *serverIndex = R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <title>Anton: Home</title>
  </head>
  <body>
    <h1>Particulate Sensor</h1>
    Measured %d seconds ago, reported %d seconds ago; last result was %s.
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

    <h1>Environment</h1>
    <dl>
      <dt>Temperature (&deg;C)</dt>
      <dd>%d</dd>

      <dt>Humidity (&#37;RH)</dt>
      <dd>%d</dd>
      
      <dt>IAQ</dt>
      <dd>%d</dd>

      <dt>CO2 (PPM)</dt>
      <dd>%d</dd>
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

    <h1>Calibration</h1>
    Visit the <a href="/calibrate">calibration page</a>.

    <h1>Reset</h1>
    Visit the <a href="/reset">reset page</a>.
    <footer>
      <hr>
      anton: revision %s
    </footer>
  </body>
</html>
)";

const char *serverUnconfigured = R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <title>Anton: Unconfigured</title>
  </head>
  <body>
    <h1>Unconfigured</h1>
    Visit the <a href="/config">config page</a> to perform configuration.
  </body>
</html>
)";

const char *calibrationPage = R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <title>Anton: Calibration</title>
  </head>
  <body>
    <a href="/">&lt; Back</a>
    <h1>Calibration</h1>
    <form method="POST" action="/calibrate-confirm">
      <input type="submit" value="Calibrate">
    </form>
  </body>
</html>
)";

const char *calibrationResultPage = R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <title>Anton: Calibration</title>
  </head>
  <body>
    <a href="/">&lt; Back</a>
    <h1>Calibration Result</h1>
    <ul>
      <li>CO2: %s</li>
    </ul>
    <p>Please leave the sensor in a neutral location for 20 minutes.</p>
  </body>
</html>
)";

const char *resetPage = R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <title>Anton: Reset</title>
  </head>
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
    <h1>Hard Reset</h1>
    This will erase all EEPROM and reboot; recommended only when all else fails.
    <form method="POST" action="/hard-reset-confirm">
      <input type="submit" value="Reset All">
    </form>
  </body>
</html>
)";
