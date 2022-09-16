Data Formats
============

There are many different on-the-wire data formats for IoT devices, including multiple standards, as well as countless custom formats.

Telemetry and device data
-------------------------

### Minimum data point requirements

Sensor data points ultimately need several things recorded in the back end, plus at least one bit of metadata:
* Device or sensor the reading is from, e.g. could be the serial number
* Which specific sensor, if there are multiple (e.g. temperature vs battery) - sometimes combined with the device ID for a single data point identifier
* Time of the reading
* The value 
* Units of the value (metadata)

The device or sensor sometimes needs to be inferred from the connection (i.e. may not be part of the main payload), and devices may not have real time clocks, so the time of reading may have to be set by the server as the time it was received.

In addition to the above, the units of the value also need to be known, either as implicit metadata, set by the standard, or otherwise specified (such as in the property name).

#### Batched data

For Low-Power Wide-Area Network (LPWAN) and other battery powered devices to conserve battery the sampleling frequency and transmit frequency may be different.

e.g. a water level sensor may sample every fifteen minutes, but only transmit once a day (or transmit immediately if alert thresholds are crossed)

This means the tranmission will include a batch of multiple values at different time points (and for devices without a real time clock the time points might all be relative to the current time, such as (seconds) -900, -1800, -2700, etc).

### Sensor vs device data

A given device may send telemetry for the different sensors it has, e.g.:
* Temperature, e.g. in degrees Celsius
* Humidity, e.g. relative percentage
* Air pressure, e.g. in Pascals

There may also be telemetry related to the device health itself, e.g.:
* Battery, e.g. voltage (in Volts)
* Signal strength, e.g. in dBm

Telemetry will often need both the current (most recent) value, as well as a time series of the history. 

### Device metadata

There is also various device metadata that is useful for classifying and organising devices, such as:
* Manufacturer
* Model
* Serial number or similar (could be several, e.g. IMEI, IMSI)
* Firware version (could be several, e.g. low level firmware vs up level software; hardware version could also be useful)
* Location

This could be transmitted by the device on first connection, or on request; or it could be entire configure out of band based on something like the serial number.

Note that in some cases **Location** (or other attributes) could be metadata, such as where a fixed sensor is installed, and sometimes could be telemetry, such as the location of a GPS sensor.

Device lifecycle
----------------

Bootstrapping

Failure handling

Commands

Configuration-over-the-air - even sensor only devices may have configuration to change sampling interval, transmit interval, and alarm thresholds.


Firmware-over-the-air

