Sensor Measurement Lists (SenML), RFC 8428, data format
=======================================================

References:
* SenML specification <https://www.rfc-editor.org/rfc/rfc8428>
* Fetch and patch extensions <https://www.rfc-editor.org/rfc/rfc8790>
* Additional units <https://www.rfc-editor.org/rfc/rfc8798>

Primarily a data format only. It has encodings in JSON, CBOR, XML, and others, and able to be sent over HTTP, CoAP, or other transports.

It does not define any device lifecycle, security, or other aspects; just the data format.

Telemetry and device data
-------------------------

The data format is minimalistic, but covers all the key aspects: measurement ID (combined device + sensor), the time, value, and units.

```json
[
     {"n":"urn:dev:ow:10e2073a01080063","u":"Cel","t":1.276020076e+09,"v":23.1}
]
```

It supports either point-in-time values or integral sum values (or both), and has a next-update-time useful for detecting missing/failed updates. You can also send string, boolean, and binary data values, so this could be used for device metadata. You could also use it for downstream communication, such as setting a transmit interval or turning an actuator on or off.

To support sending multiple values, the format supports base values, such as setting the device ID, and then each data point append the individual sensor value.

```json
[
  {"bn":"urn:dev:ow:10e2073a01080063:","n":"voltage","u":"V","v":120.1},
  {"n":"current","u":"A","v":1.2}
]
```

### Units

Standard units are listed in an IANA registry and deliberately kept limited. In general there is only one official unit for each type of measurement, e.g. for length the standard unit is "m" (meter). This is done to deliberately reduce conversion and allow values to be directly compared (instead of having cm, km, mile, etc).

Units can also be implicit, agreed out-of-band, so that they do not need to be included in the transmitted data.

### Batch data

Time values can be omitted, indicating 'now' (as in the above example), a specific point in time (unix seconds), or a relative time. Using base values and relative time, a device can efficiently sent batches of data, even without a clock.

```json
[
  {"bn":"urn:dev:ow:10e2073a01080063:voltage","bu":"V","v":120.5},
  {"t":-60,"v":120.4},
  {"t":-120,"v":120.3},
  {"t":-180,"v":120.2},
  {"t":-240,"v":120.1},
  {"bn":"urn:dev:ow:10e2073a01080063:current","bu":"A","bver":1.5},
  {"t":-60,"v":1.4},
  {"t":-120,"v":1.3},
  {"t":-180,"v":1.2},
  {"t":-240,"v":1.1}
]
```

### Resolved records

Resolved data has been normalised, by removing all base fields and relative times.

If implicit units are used they should be added, and any non-standard units should also be converted to standard units. If any values, such as a the device ID, are implict, they should also be added.

This would allow records to be used individually, such as store in a normalised database.

```json
[
  {"n":"urn:dev:ow:10e2073a01080063","u":"%RH","t":1.320067464e+09,"v":20},
  {"n":"urn:dev:ow:10e2073a01080063","u":"lon","t":1.320067464e+09,"v":24.30621},
  {"n":"urn:dev:ow:10e2073a01080063","u":"lat","t":1.320067464e+09,"v":60.07965},
  {"n":"urn:dev:ow:10e2073a01080063","u":"%RH","t":1.320067524e+09,"v":20.3},
  {"n":"urn:dev:ow:10e2073a01080063","u":"lon","t":1.320067524e+09,"v":24.30622},
  {"n":"urn:dev:ow:10e2073a01080063","u":"lat","t":1.320067524e+09,"v":60.07965},
  {"n":"urn:dev:ow:10e2073a01080063","u":"%RH","t":1.320067584e+09,"v":20.7},
  {"n":"urn:dev:ow:10e2073a01080063","u":"lon","t":1.320067584e+09,"v":24.30623},
  {"n":"urn:dev:ow:10e2073a01080063","u":"lat","t":1.320067584e+09,"v":60.07966},
  {"n":"urn:dev:ow:10e2073a01080063","u":"%EL","t":1.320067614e+09,"v":98},
  {"n":"urn:dev:ow:10e2073a01080063","u":"%RH","t":1.320067644e+09,"v":21.2},
  {"n":"urn:dev:ow:10e2073a01080063","u":"lon","t":1.320067644e+09,"v":24.30628},
  {"n":"urn:dev:ow:10e2073a01080063","u":"lat","t":1.320067644e+09,"v":60.07967}
]
```
