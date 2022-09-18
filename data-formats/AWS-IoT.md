AWS
===

Device shadows
--------------

Message published by device:

```json
{
  "state": {
    "reported": {
      "temperature": 68,
      "noise": 10
    }
  }
}
```

Sending a command:

```json
{
  "state": {
    "desired": {
      "hvacStatus": "COOLING"
    }
  }
}
```

Merged shadow aggregated state:

```json
{
  "state": {
    "reported": {
      "temperature": 68,
      "noise": 10
    },
    "desired": {
      "hvacStatus": "COOLING"
    }
  }
}
```


MQTT topics
-----------

AWS has some general guidelines for creating MQTT topics: <https://docs.aws.amazon.com/whitepapers/latest/designing-mqtt-topics-aws-iot-core/designing-mqtt-topics-aws-iot-core.html>

This include distguishing data topics from command topics and include the thing name (device ID) in any topics for publishing or subscribing. Including the device ID can be useful for routing (e.g. the device only needs to subscribe to limited  topics).

* AWS device shadows subscribe and publish to topics like `$aws/things/{thingName}/shadow/update`
* Generic structure for AWS topics for data is `dt/<application>/<context>/<thing-name>/<dt-type>`, and for commands is `cmd/<application>/<context>/<destination-id>/<req-type> ` for requests and `cmd/<application>/<context>/<destination-id>/<res-type>` for responses.


