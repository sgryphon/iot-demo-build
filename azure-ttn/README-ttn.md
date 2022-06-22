The Things Network integration with Azure IoT
=============================================

The Things Network has built in integration to Azure IoT.


Basic connection to IoT Hub
---------------------------




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



