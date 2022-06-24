IoT demo build code
===================

Sample code for building IoT demonstration components.

Sections
--------

* Core Azure IoT
  - `azure-landing` : Landing zone for IoT demos, with networking (including IPv6) and Azure Monitor.
  - `azure-iot` : Core IoT resources -- IoT Hub, Device Provisioning Service (DPS), Azure Data Explorer (ADX).

* Azure IoT demos  
  - `azure-ttn` : For connecting a LoRaWAN device (Dragino LDDS75) via The Things Network (TTN) to IoT Hub, with visualisation in ADX (from the core resources).

* Other Azure
  - `azure-leshan` : Deploy an Eclipse Leshan server, with a Caddy proxy for automatic Let's Encrypt TLS certificates, and with Basic Auth security for the admin console. Connected to the landing zone network and supports IPv6.

To Do
-----

* AWS examples
* Local device firmware examples
