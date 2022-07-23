The Things Network integration with Azure IoT
=============================================

The Things Network has built in integration to Azure IoT.


Basic connection to IoT Hub
---------------------------

First ensure the IoT Hub is deployed, from the `azure-iot/infrastructure` folder, run `deploy-iothub.ps1`.

The TTN Azure IoT Hub integration uses the SharedAccessKeyName iothubowner, and you need
to provide the IoT Hub hostname and Iot Hub access key (the primary key for iothubowner).

You can output these values via PowerShell and Azure CLI:

```powershell
$iotName = "iot-hub001-0x$((az account show --query id --output tsv).Substring(0,4))-dev"
az iot hub show --name $iotName --query properties.hostName --output tsv
az iot hub policy show --hub-name $iotName --name iothubowner --query primaryKey --output tsv
```

Open The Things Network console, then open up (or create) your demo application, e.g. xyz-iotdemo-001. Go to Integrations > Azure IoT, and then expand the Azure Iot Hub section.

Copy in the Azure IoT Hub hostname and Azure IoT Hub access key, and then click Enable Azure IoT Hub integration.

### Add devices

From the left hand menu select End devices, then click Add end device.

Enter the details, e.g. I have a Dragino LDDS75, along with the DEV EUI identifier and the APP KEY shared secret, and your local region and frequency plan, and click Register end device

Turn your device on to start sending data.

To see the data (you may need to wait, e.g. the Dragino only sends every 20 minutes), you can monitor the events. Note that this uses the default consumer group, so if you get an error, make sure there are no other consumers.

```powershell
$iotName = "iot-hub001-0x$((az account show --query id --output tsv).Substring(0,4))-dev"
az iot hub monitor-events -n $iotName --timeout 0
```

#### TTN data formatter

In the case of the Dragino LDDS75, I also had to update the Payload Formatter, as the default Device Repository formatter was for an older 6-byte message, not the 8-byte message from my update firmware.

The raw values are integers, representing millivolts, millimeters, and decidegrees Celsius. These are converted to SI base units (the same used in SenML) as volts, metres, and degrees Celsius.

The decoder output is rendered as a dictionary, so we use simple named values. To remove ambiguity the units have been specified in the field name, e.g. "distance_metres", rather than just "distance".

Alternatively the semantic interpretation could either be implicit (e.g. we agree to use SI), or we could use a reference schema that supports unit definition (e.g. DTDL) to define the units, but having explicit units is good practice that can help avoid mistakes.

There are also several options for choice of naming; the formatter below avoids abbreviations and uses snake casing (underscore), which is consistent with the rest of the uplink data message as formatted by TTN. The international (SI) spelling is used for "metre", which is consistent with both SenML and DTDL.

Note that arrays are not supported, so we cannot use formats such as SenML [RFC 8428](https://datatracker.ietf.org/doc/html/rfc8428), which supports explict units.


```javascript
function decodeUplink(input) {
  // Decode an uplink message from a buffer
  // (array) of bytes to an object of fields.
  var value = (input.bytes[0]<<8 | input.bytes[1]) & 0x3FFF;
  var battery_volts = value/1000; // Battery, units: mV -> V

  value = input.bytes[2]<<8 | input.bytes[3];
  var distance_metres = value/1000; // Distance, units: mm -> m

  var was_interrupt = !!input.bytes[4]; 

  value=input.bytes[5]<<8 | input.bytes[6];
  if(input.bytes[5] & 0x80) { value |= 0xFFFF0000; }
  var temperature_celsius = (value/10); // DS18B20 temperature, units: dCel -> Cel

  var sensor_detected = !!input.bytes[7];

  return {
    data: {
      battery_volts: battery_volts,
      distance_metres: distance_metres,
      interrupt: was_interrupt,
      temperature_celsius: temperature_celsius,
      sensor: sensor_detected
    },
    warnings: [],
    errors: []
  };
}
```

Also see:
- "To camelcase or under_score", Binkley et al, 2009, https://ieeexplore.ieee.org/document/5090039
- "An Eye Tracking Study on camelCase and under_score Identifier Styles", Sharif, 2010, https://ieeexplore.ieee.org/document/5521745


### Data explorer (ADX)

Open Azure Data Explorer (ADX), and log in. https://dataexplorer.azure.com/

Connect to your cluster, created by the azure-iothub deployment script, e.g. https://dec0xacc5dev.australiaeast.kusto.windows.net

Select the `dedb-iothub-dev-001` database, and then you can query the raw ingestion table to check if the messages are coming through:

```
['IotHub001-raw'] | take 10
```

ADX is configured to ingest all events from IoT Hub as raw data into a single table, in line with architecture recommendations from Microsoft. We can then use **ADX update policy** to split and parse the data into operational tables.

You can also query the raw data directly:

```
['IotHub001-raw']
| where tostring(rawevent.uplink_message.version_ids.model_id) == 'ldds75'
| project
    ReceivedAt = todatetime(rawevent.uplink_message.received_at),
    DeviceId = tostring(rawevent.end_device_ids.device_id),
    TtnApplication = tostring(rawevent.end_device_ids.application_ids.application_id),
    ModelId = tostring(rawevent.uplink_message.version_ids.model_id),
    DistanceM = toreal(rawevent.uplink_message.decoded_payload.distance_metres),
    BatteryV = toreal(rawevent.uplink_message.decoded_payload.battery_volts),
    RawData = base64_decode_toarray(tostring(rawevent.uplink_message.frm_payload))
| take 10
```

### ADX Dashboard visualisation

In ADX, you can create a basic dashboard, direct from this raw data.

In Azure Data Explorer, go to Dashboards, then click Create new dashboard, then give it a name, e.g. 'Dragino Demo'.

Click Add tile, and then you will need to add "+ Data Source". Call the data source `dedb-iothub-dev-001`, put in the cluster address, e.g. `https://dec0xacc5dev.australiaeast.kusto.windows.net`, click connect, select the corresponding database, and click Apply.

A production system would use ADX update policy to split and parse the data, and then materialized views to pre-calculate aggregates, but for this demo enter a query to pull data from the raw ingestion table:

```
['IotHub001-raw']
| where tostring(rawevent.uplink_message.version_ids.model_id) == 'ldds75'
| project
    ReceivedAt = todatetime(rawevent.uplink_message.received_at),
    DeviceId = tostring(rawevent.end_device_ids.device_id),
    DistanceM = toreal(rawevent.uplink_message.decoded_payload.distance_metres),
    BatteryV = toreal(rawevent.uplink_message.decoded_payload.battery_volts)
| take 720
```

Click Run to check the data, then Add visual.

Enter Tile name 'LDDS75' and Visual type Line chart. It should infer reasonable X, Y, and Series columns. Click Apply Changes to add the visual to the dashboard, then click Save in the top right.


### ADX update policy

TODO : table schema, update policy, materialised view (average distance)


### Azure Synapse

TODO: iot core - provision storage (Azure Data Lake Storage Gen2), iot hub routing (all) to storage, deploy synapse engine




### Troubleshooting

If there are issues, you can check diagnostic logs in IoT Hub on the left hand menu go to Monitoring > Logs. To see recent diagnostics you can use a query like `AzureDiagnostics | order by TimeGenerated desc`.

ResultType error codes are documented at: https://docs.microsoft.com/en-us/azure/iot-hub/troubleshoot-error-codes


Two way integration
-------------------

IoT Hub events for DeviceLifecycleEvents and TwinChangeEvents are routed to event hub, which then triggers an Azure function (hosted in an app service plan) that forwards the events to The Things Network application. The function app is configured with the TTN API KEY.

Events includes both property changes and device provisioning.

It allows device registrations to be created in IoT Hub, which are then forwarded to TTN. If they have the correct properties they will then be created as LoRaWAN devices.


TODO: Implement this


Aside: TTN template
-------------------

The Things Network provides an Azure template that can be used to create an IoT Hub configured to integrate with TTN.

The guide is here: https://www.thethingsindustries.com/docs/integrations/cloud-integrations/azure-iot-hub/deployment/

This is good for a quick proof of concept in a stand alone hub, and it's contents were used as the base for the integration above.


Aside: IoT Central
------------------

TTN also has integration to Azure IoT Central, which works for a demonstration.

However, after I recreated the IoT Central instance my device would not longer connect (from TTN upstream to Azure).

Trying to fully clear (delete the integration and device from TTN and recreate), it still failed to connect.

It could be related to the DPS, or other internals of IoT Central, being hidden and not accessible.


