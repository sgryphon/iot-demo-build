Lightweight Machine-to-Machine (LwM2M) data format
==================================================

References:
* Specification overview, with links to download <https://omaspecworks.org/what-is-oma-specworks/iot/lightweight-m2m-lwm2m/>
* Object and resource registry <https://technical.openmobilealliance.org/OMNA/LwM2M/LwM2MRegistry.html>
* <http://devtoolkit.openmobilealliance.org/OEditor/default.aspx>

LwM2M is a very complete system, covering device lifecycle with multiple bootstrapping options, fault recovery, commands, configuration, firware updates, data transmission, a full library of semantics, and is extensible. Communication protocols are very efficient and suitable for LPWAN and other battery powered devices.

Telemetry and device data
-------------------------

The logical model LwM2M data consists of object types, object instances (if multiple), with each object having individual resource types, with potentially multiple resource instances. Objects and resources are given numeric IDs, with a large library of standard semantics available.

For example, </3/0/1> represents the model, while </3303/0/5700> represents the current value of the first temperature sensor.

LwM2M supports multiple payload formats, and communication methods, include SenML JSON.

### SenML payload

Example payload with device information:
* Default minimum </1/0/2> and maximum </1/0/3> period for sending Observations
* Observations are stored while offline </1/0/6>
* Manufacturer </3/0/0>
* Model </3/0/1>
* Serial number (IMEI) </3/0/2>
* Firmware version </3/0/3>
* First available power source type </3/0/6/0> (1 = internal battery)
* Define the units for the temperature </3303/0/5701>, humidity </3304/0/5701>, and barometer </3315/0/5701> sensors (in this case matching the SenML standard units)

```json
[
  {"bn":"/1/0/","n":"2","v":60},
  {"n":"3","v":60},
  {"n":"6","vb":true},
  {"bn":"/3/0/","n":"0","vs":"M5"},
  {"n":"1","vs":"M5Atom"},
  {"n":"2","vs":"861536030196001"},
  {"n":"3","vs":"1.2.0"},
  {"n":"6/0","v":1},
  {"bn":"/","n":"3303/0/5701","vs":"Cel"},
  {"n":"3304/0/5701","vs":"%RH"},
  {"n":"3315/0/5701","vs":"Pa"}
]
```

Sensor and device health readings:
* First available power source (e.g. battery) voltage </3/0/7/0>
* Signal strength </4/0/2>
* Temperature sensor value </3303/0/5700>
* Humidity sensor value </3304/0/5700>
* Barometer sensor value </331/0/5700>

```json
[
  {"bn":"/","n":"3/0/7/0","v":5100},
  {"n":"4/0/2","v":-60.1},
  {"n":"3303/0/5700","v":23.1},
  {"n":"3304/0/5700","v":50.1},
  {"n":"3315/0/5700","v":1001}
]
```

#### Semantic differences

There are some differences between standard SenML and the LwM2M model, both in the slight modifications (e.g. adding the `vlo` property):

**Relative name**

SenML specifies that name should be the globally unique identifier, however LwM2M examples use it for the object/resource path only, which is relative to the specific device (not global).

To be globally unique the device identifier (such as the LwM2M client ID) would need to be prepended.

**Different units**

Some LwM2M resources have defined units for the values that are different from the SenML standard units list. The LwM2M specification does not include the `u` or `bu` attributes, and defines `v` as the numeric value of the LwM2M resource for types Integer, Float, or Time.

Examples:
* Battery voltage </3/0/7> in LwM2M is defined in millivolts, which is not a standard SenML unit; the standard SenML unit for voltage is volts, i.e. 5,100 mV vs 5.1 V.
* Signal strength </4/0/2> in LwM2M is defined in dBm (decibels relative to one milliwatt), whereas the standard SenML unit is dBW. To convert subtract 30, e.g. -60.1 dBm vs -90.1 dBW.

The examples in the LwM2M documentation, e.g. Table 7.4.6 of Core has `{"n":"7/0","v":3800}`, indicate (based on the value) that non-standard implied units are being used of the LwM2M defined units, e.g. mV, and ignore any standard SenML units.

Ambiguity could be avoided by explicitly specifying the units, although a LwM2M server probably won't understand the `u` attribute, e.g.

```json
[
  {"bn":"/","n":"3/0/7/0","u":"V","v":5.1},
  {"n":"4/0/2","u":"dBW","v":-90.1},
  {"n":"3303/0/5700","u":"Cel","v":23.1},
  {"n":"3304/0/5700","u":"%RH","v":50.1},
  {"n":"3315/0/5700","u":"Pa","v":1001}
]
```

For values that don't have defined units, e.g. `5700` sensor value, the `u` parameter might have been useful, but is probably not understood by the server; instead you can send the unit definition via the `5701` resource.

**Structural differences**

For sensor readings, LwM2M defines reusable resources for sensor units and (measurement) timestamp, corresponding to sensor values, whereas SenML includes the time and units in the data rows. Something like the following would be redundant, or at the worst inconsistent:

```json
[
  {"n":"3303/0/5700","u":"Cel","t":1663286980,"v":23.1},
  {"n":"3303/0/5701","vs":"Cel"},
  {"n":"3303/0/5518","v":1663286980},
]
```

### Batched data

Batched data is supported for where observations are stored while offline. LwM2M specifies breadth-first traversal of hierarchies, although it is not clear how this relates to time values.

```json
[
  {"bn":"/3/0/7/0","v":5103},
  {"t":-60,"v":5102},
  {"t":-120,"v":5101},
  {"bn":"/4/0/2","v":-60.3},
  {"t":-60,"v":-60.2},
  {"t":-120,"v":-60.1},
  {"bn":"/3303/0/5700","v":23.3},
  {"t":-60,"v":23.2},
  {"t":-120,"v":23.1},
  {"bn":"/3304/0/5700","v":50.3},
  {"t":-60,"v":50.2},
  {"t":-120,"v":50.1},
  {"bn":"/3315/0/5700","v":1003}
  {"t":-60,"v":1002},
  {"t":-120,"v":1001},
]
```

MQTT transport
--------------

The recent versions of LwM2M support MQTT as a transport.

The topic structure to use is: [ PREFIX "/" ] "lwm2m/" ( "bs" / "rd" ) "/" ENDPOINT

* PREFIX is an optional prefix, for a multi-tenant environment.
* `bs` is for the boostrap interface and `rd` for all other interfaces.
* ENDPOINT is the LwM2M endpoint client name (or if not available, then the security protocol identifier is used)

With the publish/subscribe model:
* Servers subscribe to "{PREFIX/}lwm2m/rd/#" to receive messages from all clients.
* Servers publish to "{PREFIX/}lwm2m/rd/{ENDPOINT}" for specific target endpoints.
* Clients subscribe to "{PREFIX/}lwm2m/rd/{ENDPOINT}" to receive messages from the server that are directed at them.

The Transport specification also lists clients as publishing to "{PREFIX/}lwm2m/rd/{ENDPOINT}", which would mean they receive their own messages.

Data is encoded in CBOR, with a structure depending on the operation, e.g. for Send:

```
{
  operation => 24, ; Send
  token => uint,
  ct => uint, ; LwM2M CBOR, SenML CBOR or SenML JSON
  payload => bstr, ; Updated Value
}
```



Device lifecycle
----------------

LwM2M has full support for device configuration over the air, as well as firmware updates over the air.

Multiple bootstrapping methods are supported, including full specifications for retry sequences with back offs, multiple retries, and then fallback to re-registration if needed (and you can also manually trigger re-registration).



Idea for LwM2M metadata
------------------------

There is some metadata in LwM2M that is currently stored outside the object/resource system, related to metadata Attributes, Observation requests and Send configuration (unrequested observations).

Similar to how SQL surfaces metadata (tables, columns, etc) via SQL (system) tables, it would be useful for LwM2M to surface this metadata through a set of system objects and resources.


