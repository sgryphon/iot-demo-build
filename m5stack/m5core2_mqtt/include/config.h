#pragma once

#define ST(A) #A
#define STR(A) ST(A)

#define WIFI_SSID STR(PIO_WIFI_SSID)
#define WIFI_PASSWORD STR(PIO_WIFI_PASSWORD)
#define MQTT_SERVER "mqtt-0xacc5-dev-ipv4.australiaeast.cloudapp.azure.com"
#define MQTT_PORT 8883
#define MQTT_USER STR(PIO_MQTT_USER)
#define MQTT_PASSWORD STR(PIO_MQTT_PASSWORD)
