Data Formats
============

There are many different on-the-wire data formats for IoT devices, including multiple standards, as well as countless custom formats.

As well as handling the sensor telemetry (e.g. temperature, humidity, etc), other device-to-cloud information can include device status (signal strength, battery level, etc), and device characteristics (manufacturer, model, etc).

A complete solution also needs to cover other interactions and cloud-to-device messages, including initial provisioning (bootstrapping, device lifecycle, security setup, etc), commands (activating end devices), configuration over-the-air (send interval, etc), and firmware over-the-air updates.

Some published standards that cover data formats, as well as some of the other aspects, include:

* [Lightweight Machine-to-Machine (LwM2M)](LwM2M.md)
* [OneM2M](OneM2M.md)
* [Open Connectivity Foundation (OCF)](OCF.md)
* [SparkPlug](SparkPlug.md)
* [Smart Applications REFerence Ontology (SAREF)](SAREF.md)
* [Matter](Matter.md)
* [SenML](SenML.md)

Major cloud providers also have their own default data formats for their IoT Platforms:

* [Azure IoT](Azure-IoT.md)
* [AWS IoT](AWS-IoT.md)

Rather than working down from a specific standard, it is also possible to work upwards from specific models that have already been implemented. One such approach is Smart Data Models, which collects and categorises existing data models (often references to standards such as OCF and SAREF), https://smartdatamodels.org/.


Telemetry and device data
-------------------------

### Minimum data point requirements

Sensor data points ultimately need several things recorded in the back end (or as known metadata):

* Data point identifier
  - Device or sensor the reading is from, e.g. could be the serial number
  - Which specific sensor, if there are multiple (e.g. temperature vs battery) -- sometimes combined with the device ID for a single data point identifier
* Time of the reading
* Value
  - Numeric, or other (e.g. co-ordinates, binary switch, etc), reading
  - Units of the value (may be only as metadata)

Not all of these may be transmitted, sometimes the devices only transmit the values and the device or sensor needs to be inferred from the connection (i.e. may not be part of the main payload).

Some devices may not have real time clocks, so the time of reading may have to be set by the server as the time it was received.

The units of the value also need to be known, either as implicit metadata, set by the standard, or otherwise specified (such as in the property name).

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


Messages to devices
-------------------

### Commands

For devices with attached machinery the system may need to send commands to devices, such as to turn equipment on or off, or set operating levels.

Not all devices have external state that needs to be controlled; some are sensors only.

Even sensor-only devices may have commands for operations such as taking sensor readings on demand.

### Configuration over-the-air (COTA)

Devices, including sensor-only devices, may also have configuration data that needs to change, such sampling interval, transmit interval, and alarm thresholds.

Not all devices support CotA. Some devices can only be configured directly (either factory or field configuration), and some may have local control interfaces for direct configuration by the end user.

There is a lot of similarity between commands and CotA, and some data formats use the same representation for both (e.g. desired state).


Device lifecycle
----------------

Device lifecycle can involve both device and server initiated operations.

### Bootstrapping

Devices need an initial configuration for the connection mechanism they are going to use and the server they need to communicate with.

Some devices can only be factory (or field) bootstrapped directly, and this can depend on their connection mechanism. e.g. a hardwired device may use Internet Protocol to auto-discover addresses and servers.

Wireless devices need connection information, which may be provided by a secondary channel (e.g. bluetooth).

Many devices and data protocols support bootstrapping mechanisms and processes where devices first connect to a bootstrap server for their initial configuration of the actual data server they need to connect to.

Security is an important aspect of bootstrapping, depending on the identified threat model, to protect against compromised devices.

### Failure handling

As well as telemetry transmission, data formats may also have mechanisms for fault handling, in sensors, devices, connections, and also in bootstrap and other situations.

e.g. Repeated failure to connect to the configured server may trigger a device returning to bootstrap mode.

### Firmware over-the-air (FOTA)

For long lived field devices, the ability for a protocol to support remote firmware upgrade can be an important cost saving.


