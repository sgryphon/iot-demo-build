IoT demo build code
===================

Sample code for building IoT demonstration components.


Sections
--------

* Core Azure IoT
  - [azure-landing](azure-landing/README-landing.md) : Infrastructure scripts for a Landing zone subscription for IoT demos, with networking (including IPv6) and Azure Monitor.
  - [azure-iot](azure-iot/README-iot.md) : Infrastructure scripts for core IoT resources -- IoT Hub, Device Provisioning Service (DPS), Azure Digital Twins (ADT), and supporting analysis resources such as Azure Data Explorer (ADX). Requires landing zone.

* Azure IoT demos  
  - [azure-ttn](azure-ttn/README-ttn.md) : For connecting a LoRaWAN device (Dragino LDDS75) via The Things Network (TTN) to IoT Hub, with visualisation in ADX (from the core resources). Requires core IoT.
  - [smart-factory](smart-factory/README-smart-factory.md) : Work in progress for a smart factory demo. No devices or telemetry yet.
    - [factory-line-twin](smart-factory/factory-line-twin/README-factory-line.md) : Ontology model based on SAREF Manufacturing, with scripts to import the model and instances into Azure Digital Twins. Requires core IoT.

* Other Azure
  - [azure-leshan](azure-leshan/README-leshan.md) : Infrastructure scripts to deploy an Eclipse Leshan server, with a Caddy proxy for automatic Let's Encrypt TLS certificates, and with Basic Auth security for the admin console. IPv6 by default and requires the landing zone.
  - [azure-mosquitto](azure-mosquitto/README-mosquitto.md) : Infrastructure scripts to create a Mosquitto MQTT server, with automatic TLS certificates (Let's Encrypt TLS certificates via Certbot) and MQTT authentication. IPv6 by default and requires the landing zone.

* AWS examples
  - [aws-landing](aws/README-aws-cdk.md) : Infrastructure scripts for a basic AWS Virtual Private Cloud (VPC) for IoT demos, with networking (including IPv6), using Cloud Development Kit (CDK).
  - [aws-leshan](aws-leshan/README-aws-leshan.md) : Infrastructure scripts to deploy an Eclipse Leshan server, with a Caddy proxy for automatic Let's Encrypt TLS certificates, and with Basic Auth security for the admin console. IPv6 by default.

* Device demos
  - [m5stack](m5stack/README-m5stack.md) : Example device applications for the M5Stack Core 2 using PlatformIO tools and the Arduino framework. So far: (1) hello world, (2) connecting to WiFi for random quotes.

* Matters / Thread
  - [matter-thread](matter-thread/README-matter-thread.md) includes a script for sending commands to a Python Matter Server

To Do
-----

* Local device firmware examples
* Mainflux: https://github.com/mainflux/mainflux
