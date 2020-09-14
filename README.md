# anton

A quick and dirty air sensor, using the [ZH03B][] particulate matter sensor.
Submits data directly to an [InfluxDB][] v1 instance, without auth.

I built this because the west coast is on fire, and I ran out of [Luftdaten][]
sensor parts. It's not pretty or full featured but it works.

[ZH03B]: https://www.winsen-sensor.com/sensors/dust-sensor/zh3b.html
[InfluxDB]: https://docs.influxdata.com/influxdb/v1.8/
[Luftdaten]: https://sensor.community/

## Requirements

Hardware:

* NodeMCUv3 or similar ESP8266-based system
* ZH03B particulate matter sensor

Software:

* InfluxDB v1.8

Wire the sensor as follows:

* Pin1: `VU`
* Pin2: `Ground`
* Pin4: `D1`
* Pin5: `D2`

A 3D printable case for Anton is available at [PrusaPrinters][].

[PrusaPrinters]: https://www.prusaprinters.org/prints/40746-case-for-anton-air-quality-influxdb

## Installation

This is built using [PlatformIO][]; refer to their docs on building/uploading.

Once uploaded and booted, a wireless network called `anton-setup` will be
created; join this network and you will be directed to a captive portal for
configuration.

Measurements are submitted to the configured database as a measurement named
`particulate_matter`. It will contain the following tags:

* `node` the sensor name

…and the following fields containing integer values:

* `p1_0` PM1.0
* `p2_5` PM2.5
* `p10_0` PM10

**Note:** If you need to change settings after initial configuration, change the
source code by uncommenting out the `reset_all();` statement in the `setup()`
method of `main.cpp`, upload the firmware, wait for it to reset, then re-comment
the line and upload. Probably I'll fix this at some point, but this is how you
do it for now.

[PlatformIO]: https://platformio.org/

## Acknowledgements

This was very quick to implement due to
[this excellent library](https://github.com/ShaggyDog18/SD_ZH03B) by
[@ShaggyDog18](https://github.com/ShaggyDog18/). Thanks tons.


## License

[GPL-3.0](./LICENSE)
