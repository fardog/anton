# anton

A quick and dirty air sensor which submits data directly to an [InfluxDB][]
v1 instance, without auth.

I built this because the west coast is on fire, and I ran out of [Luftdaten][]
sensor parts. It's not pretty or full featured but it works.

[InfluxDB]: https://docs.influxdata.com/influxdb/v1.8/
[Luftdaten]: https://sensor.community/

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
* (Optional) A CO₂ sensor:
  * [MH-Z19B][]

[ZH03B]: https://www.winsen-sensor.com/sensors/dust-sensor/zh3b.html
[PMS7003]: http://www.plantower.com/en/content/?110.html
[BME680]: https://www.bosch-sensortec.com/products/environmental-sensors/gas-sensors-bme680/
[MH-Z19B]: https://www.winsen-sensor.com/sensors/co2-sensor/mh-z19b.html

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

(Optional) Wire the MH-Z19B as follows, tested only on ESP32:

* VCC: `VU`
* GND: `Ground`
* ESP8266
  * TXD: `D5`
  * RXD: `D6`
* ESP32:
  * TXD: `GPIO18`
  * RXD: `GPIO19`

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

**Note:** currently only PM10 and PM2.5 are accounted for in AQI measurements,
following the EPA standard.

**Note:** if the AQI cannot be determined, either due to overflow or other
error, it will be omitted from the InfluxDB submission.

### Multisensor

If you have the optional multisensor connected, the following additional fields
fields will be sent; all are integers unless noted:

* `temperature_c` (float) °C
* `temperature_c_raw` (float) °C, uncompensated
* `humidity` %rh
* `pressure` hPa
* `gas_resistance` Ohm
* `iaq` EPA [IAQ][].
* `iaq_accuracy` IAQ accuracy; see notes
* `co2_equiv` Equivalent CO₂ PPM
* `co2_equiv_accuracy` Equivalent CO₂ accuracy; see notes
* `breath_voc` (float) Breath VOC
* `breath_voc_accuracy` Breath VOC Accuracy; see notes

[IAQ]: https://www.epa.gov/indoor-air-quality-iaq/introduction-indoor-air-quality

**Note:** Various `accuracy` values are provided by the Bosch Sensortec
[BSEC][bsec] library, which have the following meaning ([source][bsec-src]):

* `0` sensor is stabilizing
* `1` readings were too stable to define references
* `2` currently calibrating
* `3` calibrated

[bsec-src]: https://community.bosch-sensortec.com/t5/Question-and-answers/What-does-the-IAQ-accuracy-mean-in-BSEC/qaq-p/5935

### Dedicated CO₂ Sensor

If you have the optional dedicated CO₂ sensor connected, the following
additional integer fields will be sent:

* `co2` PPM

**Note:** when using the optional dedicated CO₂ sensor, you must provide a base
calibration every few weeks. This can be done from the dashboard's calibration
page, and should be initiated when background levels are low. If indoors it's
recommended to open a window and allow good air circulation for several minute
before performing the calibration. This calibration takes 20 minutes.

## Administration

Once configured, a webserver is exposed on port 80 which can be used to view
sensor values, update firmware, change configuration, and reboot or reset the
sensor.

## Changelog

See [CHANGELOG](./CHANGELOG.md).

## Acknowledgements

This was quick to implement due to the following excellent libraries:

* [@ShaggyDog18/SD_ZH03B](https://github.com/ShaggyDog18/SD_ZH03B)
* [@tobiasschuerg/InfluxDB-Client-for-Arduino](https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino)
* [@prampec/IotWebConf](https://github.com/prampec/IotWebConf)
* [@BoschSensortec/BSEC-Arduino-library][bsec]
* [@fu-hsi/PMS](https://github.com/fu-hsi/PMS)
* [@Erriez/ErriezMHZ19B](https://github.com/Erriez/ErriezMHZ19B)

## License

The code contained in this repo is released under the GNU [GPL-3.0](./LICENSE).

Building this project requires pulling in the various submodules under the `lib`
directory, each of which has their own license; of note the [BSEC][bsec] library
provided by [Bosch Sensortec][bosch] has a closed source binary blob provided
under a unique license which you must abide to use this software.

[bsec]: https://github.com/BoschSensortec/BSEC-Arduino-library
[bosch]: https://www.bosch-sensortec.com/software-tools/software/bsec/
