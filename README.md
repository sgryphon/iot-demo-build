IoT demo build code
===================

Sample code for building IoT demonstration components.


Sections
--------

* Core Azure IoT
  - [azure-landing](azure-landing/README-landing.md) : Infrastructure scripts for a Landing zone subscription for IoT demos, with networking (including IPv6) and Azure Monitor.
  - [azure-iot](azure-iot/README-iot.md) : Infrastructure scripts for core IoT resources -- IoT Hub, Device Provisioning Service (DPS), Azure Data Explorer (ADX). Requires landing zone.

* Azure IoT demos  
  - [azure-ttn](azure-ttn/README-ttn.md) : For connecting a LoRaWAN device (Dragino LDDS75) via The Things Network (TTN) to IoT Hub, with visualisation in ADX (from the core resources). Requires core IoT.
  - [smart-factory](smart-factory/README-smart-factory.md) : Work in progress with a smart factory ontology model based on SAREF Manufacturing, and scripts to import the model and instances into Azure Digital Twins. No devices or telemetry yet.

* Other Azure
  - [azure-leshan](azure-leshan/README-leshan.md) : Infrastructure scripts to deploy an Eclipse Leshan server, with a Caddy proxy for automatic Let's Encrypt TLS certificates, and with Basic Auth security for the admin console. IPv6 by default and requires the landing zone.
  - [azure-mosquitto](azure-mosquitto/README-mosquitto.md) : Infrastructure scripts to create a Mosquitto MQTT server, with automatic TLS certificates (Let's Encrypt TLS certificates via Certbot) and MQTT authentication. IPv6 by default and requires the landing zone.

* Device demos
  - [m5stack](m5stack/README-m5stack.md) : Example device applications for the M5Stack Core 2 using PlatformIO tools and the Arduino framework. So far: (1) hello world, (2) connecting to WiFi for random quotes.

To Do
-----

* AWS examples
* Local device firmware examples
