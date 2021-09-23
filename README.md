# anton

A quick and dirty air sensor which submits data directly to an [InfluxDB][]
v1 instance, without auth.

I built this because the west coast is on fire, and I ran out of [Luftdaten][]
sensor parts. It's not pretty or full featured but it works.

[InfluxDB]: https://docs.influxdata.com/influxdb/v1.8/
[Luftdaten]: https://sensor.community/

**Note:** the current `master` branch will not be optimal if you are using the
ZH03B particulate sensor. If you are, it's recommended you use the [tagged
v1.0.0 release][v1.0.0].

[v1.0.0]: https://github.com/fardog/anton/releases/tag/v1.0.0

## Requirements

Hardware:

* An ESP development board for ESP8266 or ESP32, such as:
  * NodeMCUv3 or similar ESP8266 platform
  * Lolin32 or similar ESP32 platform
* A particulate sensor, one of:
  * [ZH03B][]
  * [PMS7003][]; Only the PMS7003 has been tested, but other PMSX003 sensors may
    work.
* (Optional) A multisensor:
  * [BME680][] sensor and breakout board; configured for i2c

[ZH03B]: https://www.winsen-sensor.com/sensors/dust-sensor/zh3b.html
[PMS7003]: http://www.plantower.com/en/content/?110.html
[BME680]: https://www.bosch-sensortec.com/products/environmental-sensors/gas-sensors-bme680/

Software:

* InfluxDB v1.8

Wire the ZH03B as follows:

* Pin1: `VU`
* Pin2: `Ground`
* ESP8266
  * Pin4: `D4`
  * Pin5: `D3`
* ESP32
  * Pin4: `TX2` (`GPIO17`)
  * Pin5: `RX2` (`GPIO16`)

Or the PMS7003 as follows:

* Pin1: `VU`
* Pin3: `Ground`
* ESP8266
  * Pin7: `D3`
  * Pin9: `D4`
* ESP32
  * Pin7: `TX2` (`GPIO17`)
  * Pin9: `RX2` (`GPIO16`)

(Optional) Wire the BME680 as follows:

* VCC: `3v3`
* GND: `Ground`
* ESP8266
  * SCL: `D1`
  * SDA: `D2`
* ESP32
  * SCL: `SCL` (`GPIO22`)
  * SDA: `SDA` (`GPIO21`)

A 3D printable case for Anton is available at [PrusaPrinters][]. **Note:** the
case fits the ZH03B and ESP8266; for PMS7003 or ESP32, you're on your own right
now, but I'll make one in the future.

[PrusaPrinters]: https://www.prusaprinters.org/prints/40746-case-for-anton-air-quality-influxdb

## Installation

This is built using [PlatformIO][]; refer to their docs on building/uploading.

[PlatformIO]: https://platformio.org/

## Setup

Once uploaded and booted, a wireless network called `anton` will be
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
* `aqi` EPA [AQI][]

[AQI]: https://www.airnow.gov/aqi/aqi-basics/

…and the following string values:

* `aqi_contributor` the primary AQI contributor (highest value); the string will
  match the measurement name, e.g. `p2_5`

If you have the optional multisensor connected, the following additional integer
fields will be sent:

* `temperature` °C
* `humidity` %rh
* `pressure` hPa
* `gas_resistance` Ohm
* `iaq` EPA [IAQ][].

[IAQ]: https://www.epa.gov/indoor-air-quality-iaq/introduction-indoor-air-quality

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

This was quick to implement due to the following excellent libraries:

* [@ShaggyDog18/SD_ZH03B](https://github.com/ShaggyDog18/SD_ZH03B)
* [@tobiasschuerg/InfluxDB-Client-for-Arduino](https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino)
* [@prampec/IotWebConf](https://github.com/prampec/IotWebConf)
* [@SV-Zanshin/BME680](https://github.com/SV-Zanshin/BME680)
* [@fu-hsi/PMS](https://github.com/fu-hsi/PMS)

## License

[GPL-3.0](./LICENSE)
