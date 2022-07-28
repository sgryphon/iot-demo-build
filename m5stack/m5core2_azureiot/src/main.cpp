#include <Arduino.h>
//#include <M5Core2.h>

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/*
 * This is an Arduino-based Azure IoT Hub sample for ESPRESSIF ESP32 boards.
 * It uses our Azure Embedded SDK for C to help interact with Azure IoT.
 * For reference, please visit https://github.com/azure/azure-sdk-for-c.
 *
 * To connect and work with Azure IoT Hub you need an MQTT client, connecting,
subscribing
 * and publishing to specific topics to use the messaging features of the hub.
 * Our azure-sdk-for-c is an MQTT client support library, helping composing and
parsing the
 * MQTT topic names and messages exchanged with the Azure export
IOT_CONFIG_DEVICE_ID= export
IOT_CONFIG_IOTHUB_FQDN=iot-hub001-0xacc5-dev.azure-devices.net export
IOT_CONFIG_DEVICE_KEY=m5Dy2sMY9C4x15VhjZT0//Wpe/7GHkNZ4+HgUW96I4g= IoT Hub.
 *
 * This sample performs the following tasks:
 * - Synchronize the device clock with a NTP server;
 * - Initialize our "az_iot_hub_client" (struct for data, part of our
azure-sdk-for-c);
 * - Initialize the MQTT client (here we use ESPRESSIF's esp_mqtt_client, which
also handle the tcp connection and TLS);
 * - Connect the MQTT client (using server-certificate validation, SAS-tokens
for client authentication);
 * - Periodically send telemetry data to the Azure IoT Hub.
 *
 * To properly connect to your Azure IoT Hub, please fill the information in the
`iot_configs.h` file.
 */

// C99 libraries
#include <cstdlib>
#include <string.h>
#include <time.h>

// Libraries for MQTT client and WiFi connection
#include <WiFi.h>
#include <mqtt_client.h>

// Azure IoT SDK for C includes
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

// Additional sample headers
#include "AzIoTSasToken.h"
#include "SerialLogger.h"
#include "config.h"

#include <M5Core2.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#include "DemoConsole.h"
#include "StartNetwork.h"

#include "esp_log.h"
static const char *TAG = "demo";

// When developing for your own Arduino-based platform,
// please follow the format '(ard;<platform>)'.
#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING "(ard;esp32)"

// Utility macros and defines
#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0
#define SAS_TOKEN_DURATION_IN_MINUTES 60
#define UNIX_TIME_NOV_13_2017 1510592825

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF 1

#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST                                                    \
  ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

// Translate iot_configs.h defines into variables used by the sample
static const char *ssid = WIFI_SSID;
static const char *password = WIFI_PASSWORD;

#define ST(A) #A
#define STR(A) ST(A)

static const char *host = STR(IOT_CONFIG_IOTHUB_FQDN);
static const char *config_device_id = STR(IOT_CONFIG_DEVICE_ID);
static char device_id[128];

#ifndef IOT_CONFIG_USE_X509_CERT
static const char *device_key = STR(IOT_CONFIG_DEVICE_KEY);
#endif

static const int mqttPort = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT;

// Memory allocated for the sample's variables and structures.
static az_iot_hub_client client;

static char mqtt_client_id[128];
static char mqtt_username[200];
static char mqtt_password[250];
static uint8_t sas_signature_buffer[256];
static unsigned long next_telemetry_send_time_ms = 0;
static char telemetry_topic[128];
static uint8_t telemetry_payload[100];
static uint32_t telemetry_send_count = 0;

#define INCOMING_DATA_BUFFER_SIZE 128
static char incoming_data[INCOMING_DATA_BUFFER_SIZE];

//#define IOT_CONFIG_DEVICE_KEY2 "m5Dy2sMY9C4x15VhjZT0//Wpe/7GHkNZ4+HgUW96I4g="
#define IOT_CONFIG_DEVICE_KEY2 "cEnip1AOSJ34v3oqf8lb5mxoU3+rmVFFTZMWU0n4jh0="
// Auxiliary functions
#ifndef IOT_CONFIG_USE_X509_CERT
static AzIoTSasToken
    sasToken(&client, AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY2),
             // az_span_create((uint8_t*)device_key, strlen(device_key)),
             AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
             AZ_SPAN_FROM_BUFFER(mqtt_password));
#endif // IOT_CONFIG_USE_X509_CERT

WiFiClientSecure *wifiClientSecure = NULL;
PubSubClient pubSubClient;

static void connectToWiFi() {
  DemoConsole.printf("Connecting to WIFI SSID %s\n", ssid);

  StartNetwork.begin(ssid, password);

  while (!StartNetwork.wifiConnected()) {
    delay(500);
    DemoConsole.loop();
    DemoConsole.print(".");
  }

  DemoConsole.printf("WiFi connected, IP address: %s\n",
                     WiFi.localIP().toString().c_str());
}

static void initializeTime() {
  DemoConsole.print("Setting time using RTC\n");
  RTC_TimeTypeDef rtc_time;
  RTC_DateTypeDef rtc_date;
  // Read date after time to ensure consistency (reading time locks values until
  // date is read)
  M5.Rtc.GetTime(&rtc_time);
  M5.Rtc.GetDate(&rtc_date);

  struct tm time_info = {0};
  time_info.tm_year = rtc_date.Year - 1900;
  time_info.tm_mon = rtc_date.Month - 1;
  time_info.tm_mday = rtc_date.Date;
  time_info.tm_hour = rtc_time.Hours;
  time_info.tm_min = rtc_time.Minutes;
  time_info.tm_sec = rtc_time.Seconds;
  struct timespec time_spec = {0};
  time_spec.tv_sec = mktime(&time_info);
  clock_settime(CLOCK_REALTIME, &time_spec);

  /*
    DemoConsole.print("Setting time using SNTP\n");
    configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
    time_t now = time(NULL);
    while (now < UNIX_TIME_NOV_13_2017)
    {
      delay(500);
      DemoConsole.loop();
      DemoConsole.print(".");
      now = time(nullptr);
    }
  */

  DemoConsole.printf("Time initialized %d (%s)\n", time_spec.tv_sec,
                     asctime(&time_info));
}

void pubSubClientCallback(char *topic, byte *payload, unsigned int length) {
  DemoConsole.printf("Topic: %s\n", topic);
  int i;
  for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < length; i++) {
    incoming_data[i] = payload[i];
  }
  incoming_data[i] = '\0';
  DemoConsole.printf("Data: %s\n", incoming_data);
}

static void initializeIoTHubClient() {
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

  if (az_result_failed(az_iot_hub_client_init(
          &client, az_span_create((uint8_t *)host, strlen(host)),
          az_span_create((uint8_t *)device_id, strlen(device_id)), &options))) {
    DemoConsole.print("ERROR: Failed initializing Azure IoT Hub client\n");
    return;
  }

  size_t client_id_length;
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1,
          &client_id_length))) {
    DemoConsole.print("ERROR: Failed getting client id\n");
    return;
  }

  if (az_result_failed(az_iot_hub_client_get_user_name(
          &client, mqtt_username, sizeofarray(mqtt_username), NULL))) {
    DemoConsole.print("ERROR: Failed to get MQTT clientId, return code\n");
    return;
  }

  DemoConsole.printf("Client ID: %s\n", mqtt_client_id);
  DemoConsole.printf("Username: %s\n", mqtt_username);

  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL))) {
    DemoConsole.print(
        "ERROR: Failed az_iot_hub_client_telemetry_get_publish_topic\n");
    return;
  }
  DemoConsole.printf("Publish topic: %s\n", telemetry_topic);
  DemoConsole.printf("Subscribe topic: %s\n",
                     AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);
}

static int initializeMqttClient() {
  DemoConsole.print("initializeMqttClient\n");
  if (!pubSubClient.connected()) {
#ifndef IOT_CONFIG_USE_X509_CERT
    DemoConsole.print("Generating SAS token\n");

    ESP_LOGD(TAG, "Time %d", time(NULL));
    RTC_TimeTypeDef rtcTimeNow;
    M5.Rtc.GetTime(&rtcTimeNow);
    ESP_LOGD(TAG, "%02d:%02d:%02d", rtcTimeNow.Hours, rtcTimeNow.Minutes,
             rtcTimeNow.Seconds);

#endif

    DemoConsole.print("Attempting MQTT connection...\n");
    if (wifiClientSecure) {
      ESP_LOGD(TAG, "Deleting wifClientSecure");
      delete wifiClientSecure;
    }
    wifiClientSecure = new WiFiClientSecure;
    if (wifiClientSecure) {
      ESP_LOGD(TAG, "Created new wifiClientSecure");

      wifiClientSecure->setCACert((const char *)ca_pem);

#ifdef IOT_CONFIG_USE_X509_CERT
      DemoConsole.print("MQTT client using X509 Certificate authentication\n");
      // mqtt_config.client_cert_pem = IOT_CONFIG_DEVICE_CERT;
      // mqtt_config.client_key_pem = IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY;
      wifiClientSecure->setCertificate(device_client_ca);
      wifiClientSecure->setPrivateKey(device_private_key);
#else // Using SAS key
      if (sasToken.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0) {
        DemoConsole.print("ERROR: Failed generating SAS token\n");
        return 1;
      }
      DemoConsole.print("Setting password from SAS token\n");
      const char *connect_password = (const char *)az_span_ptr(sasToken.Get());
      DemoConsole.printf("\n**Got SAS password: %s\n\n", connect_password);
#endif

      pubSubClient.setClient(*wifiClientSecure);
      pubSubClient.setKeepAlive(30);
      pubSubClient.setBufferSize(1000);
      // pubSubClient.setCallback(pubSubClientCallback);

      DemoConsole.printf("Connecting to MQTT server %s, port %d\n", host,
                         mqttPort);
      pubSubClient.setServer(host, mqttPort);

      bool client_connected = wifiClientSecure->connect(host, mqttPort);
      DemoConsole.printf("wifiClientSecure connected %d\n", client_connected);

      DemoConsole.printf("Client ID %s, User %s\n", mqtt_client_id,
                         mqtt_username);
      if (pubSubClient.connect(mqtt_client_id, mqtt_username,
                               connect_password)) {
        DemoConsole.print("MQTT connected\n");
        bool subscribed =
            pubSubClient.subscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
        if (subscribed) {
          DemoConsole.printf("Subscribed for cloud-to-device messages.\n");
        } else {
          // DemoConsole.print("ERROR: Could not subscribe for cloud-to-device
          // messages.\n");
        }
      } else {
        DemoConsole.print("failed, state=");
        DemoConsole.printf("%d", pubSubClient.state());
        DemoConsole.print(", con=");
        DemoConsole.printf("%d", pubSubClient.connected());
        DemoConsole.print("\n");
        return 2;
      }
    } else {
      DemoConsole.print("Unable to create client\n");
      return 3;
    }
  }
}

/*
 * @brief           Gets the number of seconds since UNIX epoch until now.
 * @return uint32_t Number of seconds.
 */
static uint32_t getEpochTimeInSecs() { return (uint32_t)time(NULL); }

static void establishConnection() {
  connectToWiFi();

  time_t now = time(NULL);
  DemoConsole.printf("Time %d\n", now);
  RTC_TimeTypeDef rtcTimeNow;
  M5.Rtc.GetTime(&rtcTimeNow);
  M5.Lcd.printf("%02d:%02d:%02d\n", rtcTimeNow.Hours, rtcTimeNow.Minutes,
                rtcTimeNow.Seconds);

  initializeTime();
  initializeIoTHubClient();
  int result = initializeMqttClient();
  DemoConsole.printf("MQTT initialise: %d\n", result);
  if (result != ESP_OK) {
    delay(1000);
  }
}

static void getTelemetryPayload(az_span payload, az_span *out_payload) {
  az_span original_payload = payload;

  payload = az_span_copy(payload, AZ_SPAN_FROM_STR("{ \"msgCount\": "));
  (void)az_span_u32toa(payload, telemetry_send_count++, &payload);
  payload = az_span_copy(payload, AZ_SPAN_FROM_STR(" }"));
  payload = az_span_copy_u8(payload, '\0');

  *out_payload =
      az_span_slice(original_payload, 0,
                    az_span_size(original_payload) - az_span_size(payload) - 1);
}

static void sendTelemetry() {
  az_span telemetry = AZ_SPAN_FROM_BUFFER(telemetry_payload);

  DemoConsole.print("Sending telemetry ...\n");

  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to
  // reflect the current values of the properties.
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL))) {
    DemoConsole.print(
        "ERROR: Failed az_iot_hub_client_telemetry_get_publish_topic\n");
    return;
  }

  getTelemetryPayload(telemetry, &telemetry);

  // if (esp_mqtt_client_publish(
  //         mqtt_client,
  //         telemetry_topic,
  //         (const char*)az_span_ptr(telemetry),
  //         az_span_size(telemetry),
  //         MQTT_QOS1,
  //         DO_NOT_RETAIN_MSG)
  //     == 0)
  if (pubSubClient.publish(telemetry_topic, az_span_ptr(telemetry),
                           az_span_size(telemetry), DO_NOT_RETAIN_MSG)) {
    DemoConsole.print("Message published successfully\n");
  } else {
    DemoConsole.print("ERROR: Failed publishing\n");
  }
}

// Arduino setup and loop main functions.

void setup() {
  Serial.begin(115200);
  ESP_LOGI(TAG, "** Setup **");

  M5.begin();
  DemoConsole.begin();

  bool missingConfig = false;
  if (ssid == "") {
    DemoConsole.print("WiFi SSID missing\n");
    missingConfig = true;
  } else {
    DemoConsole.printf("Using WiFi SSID: %s\n", ssid);
  }
  if (password == "") {
    DemoConsole.print("WiFi Password missing\n");
    missingConfig = true;
  }
  if (host == "") {
    DemoConsole.print("IoT Hub FQDN missing\n");
    missingConfig = true;
  } else {
    DemoConsole.printf("Using IoT Hub: %s\n", host);
  }

  if (config_device_id == "") {
    snprintf(device_id, sizeof(device_id), "eui-%s", StartNetwork.eui64());
    DemoConsole.printf("Using default Device ID: %s\n", device_id);
  } else {
    snprintf(device_id, sizeof(device_id), "%s", config_device_id);
    DemoConsole.printf("Using configured Device ID: %s\n", device_id);
  }

#ifndef IOT_CONFIG_USE_X509_CERT
  if (device_key == "") {
    DemoConsole.printf("IoT Hub Key missing");
    missingConfig = true;
  }
#endif

#ifdef IOT_CONFIG_USE_X509_CERT
  if (IOT_CONFIG_DEVICE_CERT == "") {
    DemoConsole.print("IoT Hub X509 Certificate missing");
    missingConfig = true;
  }
  if (IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY == "") {
    DemoConsole.print("IoT Hub X509 Private Key missing");
    missingConfig = true;
  }
}
#endif

if (missingConfig) {
  return;
}

establishConnection();
}

void loop() {
  ESP_LOGD(TAG, "** Loop %d **", millis());
  // ESP_LOGI(TAG, "** Loop %d **", millis());

  M5.update();
  DemoConsole.loop();

  if (StartNetwork.wifiConnected()) {
    if (!pubSubClient.connected()) {
      initializeMqttClient();
    }
#ifndef IOT_CONFIG_USE_X509_CERT
    else if (sasToken.IsExpired()) {
      DemoConsole.print("SAS token expired; reconnecting with a new one.\n");
      //(void)esp_mqtt_client_destroy(mqtt_client);
      pubSubClient.disconnect();
      initializeMqttClient();
    }
#endif
    else if (millis() > next_telemetry_send_time_ms) {
      sendTelemetry();
      next_telemetry_send_time_ms = millis() + TELEMETRY_FREQUENCY_MILLISECS;
    }
  }

  delay(500);
}
