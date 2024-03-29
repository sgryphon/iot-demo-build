Azure IoT data format
=====================

References:
* Messaging guide, <https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messaging>
* Plug and play model repository, Mhttps://github.com/Azure/iot-plugandplay-models>

Azure IoT Hub uses a very open format and can accept incoming messages in any format, however this means there are no defined semantics.

For a bit more structure, Azure IoT supports device twins (particularly through IoT Central), and has a defined structure for device to cloud and cloud to device messaging based on this, e.g. desired property changes that then get applied and reflected back.

Device models are specified in Digital Twin Definition Language (DTDL), which is a fairly comprehensive specification including simple properties, complex properties, telemetry, relationships, and inheritance, including specifying semantic type and units for properties and telemetry.


Device twin definition
----------------------

The information schema for devices is expressed in DTDL.

* Plug and play model repository, <https://github.com/Azure/iot-plugandplay-models>

```json
{
    "@context": "dtmi:dtdl:context;2",
    "@id": "dtmi:azure:DeviceManagement:DeviceInformation;1",
    "@type": "Interface",
    "displayName": "Device Information",
    "contents": [
        {
            "@type": "Property",
            "name": "manufacturer",
            "displayName": "Manufacturer",
            "schema": "string",
            "description": "Company name of the device manufacturer. This could be the same as the name of the original equipment manufacturer (OEM). Ex. Contoso."
        },
        {
            "@type": "Property",
            "name": "model",
            "displayName": "Device model",
            "schema": "string",
            "description": "Device model name or ID. Ex. Surface Book 2."
        },
        {
            "@type": "Property",
            "name": "swVersion",
            "displayName": "Software version",
            "schema": "string",
            "description": "Version of the software on your device. This could be the version of your firmware. Ex. 1.3.45"
        }
    ]
}
```

Telemetry and device data
-------------------------

Telemetry, and other basic data, can be sent as simple JSON in a device to cloud message. Note that the semantics and units for the values are well-defined by the DTDL.

```json
{
    "connectivity": {
        "signalStrength": -60.1
    },
    "sensor": {
        "temperature": 23.1,
        "humidity": 50.1,
        "pressure": 10001
    }
}
```

Updating reported properties can also be a simple message:

```json
{
    "deviceInformation": {
        "manufacturer": "M5",
        "model": "M5Atom",
        "swVersion": "1.2.0"
    },
    "connectivity": {
        "imei": "861536030196001",
    },
    "telemetryInterval": 60
}
```

More complex specific formats are used for the device twin such as receiving desired and reported properties:

```json
{
    "desired": {
        "telemetryInterval": "5m",
        "$version": 12
    },
    "reported": {
        "telemetryInterval": "5m",
        "batteryLevel": 55,
        "$version": 123
    }
}
```

Batched data
------------

Azure IoT does not have any mechanism for directly supporting batched data.

You would have to manually add a composite data structure for historical data with things like the date time; and even then it would still come in as a single message and need to be processed.


Distributed tracing support
---------------------------

TODO: Add property to trigger distributed tracing by the server.
 


MQTT Support
------------

Whilst Azure Iot supports MQTT as a transport mechanism, it is not a full MQTT broker.

The username and password fields need to contain fairly long specific formatted values (including a calculated shared access signature).

For sending and receiving messages, specific topics are used:
* Publish to `devices/{device-id}/messages/events/$.ct=application%2Fjson%3Bcharset%3Dutf-8` for sending messages with a specific content type.
* Subscribe to `devices/{device-id}/messages/devicebound/#` for receiving messages, where the received topic may include a property bag of additional values.

For modules, the values are:
* Publish to `devices/{device-id}/modules/{module-id}/messages/events/`
* Subscribe to `devices/{device-id}/modules/{module-id}/#`

Device twin messages are a bit more complicated. First, a device subscribes to `$iothub/twin/res/#`, to receive the operation's responses. Then, it sends an empty message to topic `$iothub/twin/GET/?$rid={request id}`, with a populated value for request ID. The service then sends a response message containing the device twin data on topic `$iothub/twin/res/{status}/?$rid={request-id}`, using the same request ID as the request.

The response will look something like the following:

```json
{
    "desired": {
        "telemetrySendInterval": "5m",
        "$version": 12
    },
    "reported": {
        "telemetrySendInterval": "5m",
        "batteryLevel": 55,
        "$version": 123
    }
}
```

For more details see:
* <https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support>

Note that usually you can just use the Azure IoT libraries and don't need to worry about these low level details.


