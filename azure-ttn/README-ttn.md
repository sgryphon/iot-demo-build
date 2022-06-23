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

In the case of the Dragino LDDS75, I also had to update the Payload Formatter, as the default Device Repository formatter was for an older 6-byte message, not the 8-byte message from my update firmware.

Turn your device on to start sending data.

To see the data (you may need to wait, e.g. the Dragino only sends every 20 minutes), you can monitor the events. Note that this uses the default consumer group, so if you get an error, make sure there are no other consumers.

```powershell
$iotName = "iot-hub001-0x$((az account show --query id --output tsv).Substring(0,4))-dev"
az iot hub monitor-events -n $iotName --timeout 0
```

### Data explorer

Open Data Explorer, and log in. https://dataexplorer.azure.com/

Connect to your cluster, created by the azure-iothub deployment script, e.g. https://dec0xacc5dev.australiaeast.kusto.windows.net


### Troubleshooting

If there are issues, you can check diagnostic logs in IoT Hub on the left hand menu go to Monitoring > Logs. To see recent diagnostics you can use a query like `AzureDiagnostics | order by TimeGenerated desc`.

ResultType error codes are documented at: https://docs.microsoft.com/en-us/azure/iot-hub/troubleshoot-error-codes



Two way integration
-------------------

IoT Hub events for DeviceLifecycleEvents and TwinChangeEvents are routed to event hub, which then triggers an Azure function (hosted in an app service plan) that forwards the events to The Things Network application. The function app is configured with the TTN API KEY.

Events includes both property changes and device provisioning.

It allows device registrations to be created in IoT Hub, which are then forwarded to TTN. If they have the correct properties they will then be created as LoRaWAN devices.


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


HostName=iot-hub-0xacc5-dev.azure-devices.net;SharedAccessKeyName=iothubowner;SharedAccessKey=XpKoZdbg86ryN9Cq9oqIlxQEphsph8AhWhDsl+Y1O7o=
