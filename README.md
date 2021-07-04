# anton

A quick and dirty air sensor, using the [ZH03B][] particulate matter sensor.
Submits data directly to an [InfluxDB][] v1 instance, without auth.

I built this because the west coast is on fire, and I ran out of [Luftdaten][]
sensor parts. It's not pretty or full featured but it works.

[InfluxDB]: https://docs.influxdata.com/influxdb/v1.8/
[Luftdaten]: https://sensor.community/

## Requirements

Hardware:

* NodeMCUv3 or similar ESP8266-based system
* [ZH03B][] particulate matter sensor
* [BME680][] sensor and breakout board; configured for i2c

[ZH03B]: https://www.winsen-sensor.com/sensors/dust-sensor/zh3b.html
[BME680]: https://www.bosch-sensortec.com/products/environmental-sensors/gas-sensors-bme680/

Software:

* InfluxDB v1.8

Wire the ZH03B as follows:

* Pin1: `VU`
* Pin2: `Ground`
* Pin4: `D3`
* Pin5: `D4`

Wire the BME680 as follows:

* VCC: `3v3`
* GND: `Ground`
* SCL: `D1`
* SDA: `D2`

A 3D printable case for Anton is available at [PrusaPrinters][].

[PrusaPrinters]: https://www.prusaprinters.org/prints/40746-case-for-anton-air-quality-influxdb

## Installation

This is built using [PlatformIO][]; refer to their docs on building/uploading.

[PlatformIO]: https://platformio.org/

## Setup

Once uploaded and booted, a wireless network called `anton-setup` will be
created; join this network and you will be directed to a captive portal for
configuration.

## Measurements

Measurements are submitted to the configured database as a measurement named
`particulate_matter`. It will contain the following tags:

* `node` the sensor name
* `location` (optional) the sensor location

…and the following fields containing integer values:

* `p1_0` PM1.0
* `p2_5` PM2.5
* `p10_0` PM10
* `aqi` EPA AQI

…and the following string values:

* `aqi_contributor` the primary AQI contributor (highest value); the string will
  match the measurement name, e.g. `p1_0`

**Note:** currently only PM10 and PM2.5 are accounted for in AQI measurements,
following the EPA standard.

**Note:** if the AQI cannot be determined, either due to overflow or other
error, it will be omitted from the InfluxDB submission.

## Administration

Once configured, a webserver is exposed on port 80 which can be used to view
sensor values, update firmware, change configuration, and reboot or reset the
sensor.

## Changelog

Changes which are not backwards compatible will be listed here. Anton is under
active development and does not have a fixed feature set, nor does it have
version numbers yet. For now, changes will be listed by date.

* **2021-07-04** Moves from [WifiManager][] to [IotWebConf][]. *This is a
  backwards incompatible change.* You will need to reconfigure the sensor after
  upgrading.

* **2020-10-22** Moved from the deprecated `SPIFFS` to the still-maintained
  `LittleFS`. *This is a backwards incompatible change.* You will need to
  reconfigure the sensor after upgrading.

* **2020-10-03** earlier versions of Anton submitted values to InfluxDB as
  floats; this has been corrected to use integer values. Updating will require
  you to either use a new database or rewrite your existing one.

[WifiManager]: https://github.com/tzapu/WiFiManager
[IotWebConf]: https://github.com/prampec/IotWebConf

## Acknowledgements

This was very quick to implement due to
[this excellent library](https://github.com/ShaggyDog18/SD_ZH03B) by
[@ShaggyDog18](https://github.com/ShaggyDog18/). Thanks tons.


## License

[GPL-3.0](./LICENSE)
