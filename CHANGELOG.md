# Changelog

Changes which are not backwards compatible will be listed here. Anton is under
active development and does not have a fixed feature set, nor does it have
version numbers yet. For now, changes will be listed by date.

## 2022-01-03

To report more accurate temperatures, the following changes were made:

* Temperature fields are now floats
* A compile-time temperature offset may be provided, which is used by the BSEC
  library to perform its calculations.

### Temperature Fields

Temperature fields are now floats; in previous versions they were integers,
which loses a lot of precision when dealing in Celsius. To avoid conflicts
with existing databases, fields were renamed/added:

* `temperature` is now `temperature_c`
* `temperature_c_raw` is the uncompensated temperature; direct from the sensor

**Note:** `temperature_c_raw` is _not_ the same as "`temperature_c` ignoring
offset", but rather the raw reading from the sensor itself. If you needed the
previous equivalent `temperature` you should take `temperature_c` and remove the
compiled offset from it, although there shouldn't be much use for this value.

### Compile-time Temperature Offset

When compiling Anton firmware, you may define the float value
`BSEC_TEMPERATURE_OFFSET_C` which will be passed to the underlying [BSEC][bsec]
library to determine the temperature offset that should be used. The default
value of `4.0` works well for various 3D printable cases, and you may override
it for your application as necessary.

**Note:** This value is _subtracted_ from the values that are read; you will
almost always wish to specify a positive value here.

## 2021-12-12

Move to Bosch Sensortec's [BSEC][bsec] library for the BME680, which improves
accuracy of the temperature/humidity readings as well as provides equivalent CO₂
values without the dedicated sensor. However this brings additional license
restrictions which you should [view and accept][bosch] before using.

[bsec]: https://github.com/BoschSensortec/BSEC-Arduino-library
[bosch]: https://www.bosch-sensortec.com/software-tools/software/bsec/


## 2021-09-25

Does away with pin/UART configurations and makes them compile time. Also added
CO₂ sensor options. This caused a revision of the configuration options, and you
will need to reconfigure the sensor after flashing.

## 2021-07-04

Moves from [WifiManager][] to [IotWebConf][]. *This is a backwards incompatible
change.* You will need to reconfigure the sensor after upgrading.

[WifiManager]: https://github.com/tzapu/WiFiManager
[IotWebConf]: https://github.com/prampec/IotWebConf

## 2020-10-22

Moved from the deprecated `SPIFFS` to the still-maintained `LittleFS`. *This is
a backwards incompatible change.* You will need to reconfigure the sensor after
upgrading.

## 2020-10-03

Earlier versions of Anton submitted values to InfluxDB as floats; this has been
corrected to use integer values. Updating will require you to either use a new
database or rewrite your existing one.
