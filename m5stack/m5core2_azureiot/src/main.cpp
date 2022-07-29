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

#include "DemoConsole.h"
#include "StartNetwork.h"
#include "config.h"

#include "AzIoTSasToken.h"
#include "SerialLogger.h"

// Azure IoT SDK for C includes
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

#include <Arduino.h>
#include <M5Core2.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_log.h>

// C99 libraries
#include <cstdlib>
#include <string.h>
#include <time.h>

#define ST(A) #A
#define STR(A) ST(A)

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

static const char *TAG = "demo";

// Translate iot_configs.h defines into variables used by the sample
static const char *ssid = WIFI_SSID;
static const char *password = WIFI_PASSWORD;

static const char *host = STR(IOT_CONFIG_IOTHUB_FQDN);
static const char *config_device_id = STR(IOT_CONFIG_DEVICE_ID);
static char device_id[128];

#ifndef IOT_CONFIG_USE_X509_CERT
static const char *device_key = STR(IOT_CONFIG_DEVICE_KEY);
#endif

static const int mqtt_port = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT;

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
    sas_token(&client, AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY2),
              // az_span_create((uint8_t*)device_key, strlen(device_key)),
              AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
              AZ_SPAN_FROM_BUFFER(mqtt_password));
#endif // IOT_CONFIG_USE_X509_CERT

WiFiClientSecure *wifi_client_secure = NULL;
PubSubClient mqtt_client;

static void connectToWiFi() {
  DemoConsole.writeMessage("Connecting to WIFI SSID %s", ssid);

  StartNetwork.begin(ssid, password);

  while (!StartNetwork.wifiConnected()) {
    delay(500);
    DemoConsole.loop();
    Serial.print(".");
  }

  ESP_LOGD(TAG, "WiFi connect complete, IP address: %s",
           WiFi.localIP().toString().c_str());
}

static void initializeTime() {
  DemoConsole.writeMessage("Setting time using RTC");
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

  DemoConsole.writeMessage("Time initialized %d (%s)", time_spec.tv_sec,
                           asctime(&time_info));
}

void pubSubClientCallback(char *topic, byte *payload, unsigned int length) {
  ESP_LOGD(TAG, "Received topic %s, length %d", topic, length);
  int i;
  for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < length; i++) {
    incoming_data[i] = payload[i];
  }
  incoming_data[i] = '\0';
  DemoConsole.writeMessage("Received [%s] %s", topic, incoming_data);
}

static void initializeIoTHubClient() {
  DemoConsole.writeMessage("Initializing IoT Hub client");
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

  if (az_result_failed(az_iot_hub_client_init(
          &client, az_span_create((uint8_t *)host, strlen(host)),
          az_span_create((uint8_t *)device_id, strlen(device_id)), &options))) {
    DemoConsole.writeMessage("ERROR: Failed initializing Azure IoT Hub client");
    return;
  }

  size_t client_id_length;
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1,
          &client_id_length))) {
    DemoConsole.writeMessage("ERROR: Failed getting client id");
    return;
  }

  if (az_result_failed(az_iot_hub_client_get_user_name(
          &client, mqtt_username, sizeofarray(mqtt_username), NULL))) {
    DemoConsole.writeMessage("ERROR: Failed to get MQTT clientId, return code");
    return;
  }

  DemoConsole.writeMessage("Client ID: %s", mqtt_client_id);
  DemoConsole.writeMessage("Username: %s", mqtt_username);

  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL))) {
    DemoConsole.writeMessage(
        "ERROR: Failed az_iot_hub_client_telemetry_get_publish_topic");
    return;
  }
  DemoConsole.writeMessage("Publish topic: %s", telemetry_topic);
  DemoConsole.writeMessage("Subscribe topic: %s",
                           AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);
}

static int16_t initializeMqttClient() {
  if (!mqtt_client.connected()) {
    DemoConsole.writeMessage("Initializing MQTT client");
    if (wifi_client_secure) {
      ESP_LOGD(TAG, "Deleting wifClientSecure");
      delete wifi_client_secure;
    }
    wifi_client_secure = new WiFiClientSecure;
    if (wifi_client_secure) {
      ESP_LOGD(TAG, "Created new wifiClientSecure");
      wifi_client_secure->setCACert((const char *)ca_pem);

#ifdef IOT_CONFIG_USE_X509_CERT
      DemoConsole.writeMessage("MQTT client using X509 Certificate");
      // mqtt_config.client_cert_pem = IOT_CONFIG_DEVICE_CERT;
      // mqtt_config.client_key_pem = IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY;
      wifiClientSecure->setCertificate(device_client_ca);
      wifiClientSecure->setPrivateKey(device_private_key);
#else // Using SAS key
      RTC_TimeTypeDef rtc_time_now;
      M5.Rtc.GetTime(&rtc_time_now);
      ESP_LOGD(TAG, "Generating SAS token, time %d (%02d:%02d:%02d)",
               time(NULL), rtc_time_now.Hours, rtc_time_now.Minutes,
               rtc_time_now.Seconds);
      if (sas_token.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0) {
        DemoConsole.writeMessage("ERROR: Failed generating SAS token");
        return 1;
      }
      ESP_LOGD(TAG, "Setting password from SAS token");
      const char *connect_password = (const char *)az_span_ptr(sas_token.Get());
      ESP_LOGV(TAG, "** Got SAS password: %s", connect_password);
#endif

      mqtt_client.setClient(*wifi_client_secure);
      mqtt_client.setKeepAlive(30);
      mqtt_client.setBufferSize(1000);
      // pubSubClient.setCallback(pubSubClientCallback);

      DemoConsole.writeMessage("Connecting to server %s:%d", host, mqtt_port);
      mqtt_client.setServer(host, mqtt_port);

      bool client_connected = wifi_client_secure->connect(host, mqtt_port);
      if (client_connected) {
        DemoConsole.writeMessage("Authenticating to MQTT as client ID %s",
                                 mqtt_client_id);
        if (mqtt_client.connect(mqtt_client_id, mqtt_username,
                                connect_password)) {
          DemoConsole.writeMessage("MQTT connected");
          bool subscribed =
              mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
          if (subscribed) {
            DemoConsole.writeMessage("Subscribed for cloud-to-device messages");
          } else {
            DemoConsole.writeMessage(
                "ERROR: Could not subscribe for cloud-to-device messages");
          }
        } else {
          DemoConsole.writeMessage("ERROR: MQTT state %d, connected %d",
                                   mqtt_client.state(),
                                   mqtt_client.connected());
          return 2;
        }
      } else {
        DemoConsole.writeMessage("ERROR: Server connection failed");
        return 3;
      }
    } else {
      DemoConsole.writeMessage("ERROR: Unable to create secure client");
      return 4;
    }
  }
  return 0;
}

static void establishConnection() {
  connectToWiFi();
  initializeTime();
  initializeIoTHubClient();
  int result = initializeMqttClient();
  ESP_LOGD(TAG, "MQTT initialise result: %d", result);
  if (result != ESP_OK) {
    delay(1000);
  }
}

static void getTelemetryPayload(az_span payload, az_span *out_payload) {
  az_result rc;
  az_json_writer jw;
  rc = az_json_writer_init(&jw, payload, NULL);
  if (az_result_failed(rc))
    ESP_LOGE(TAG, "Telemetry payload failed %d", rc);
  rc = az_json_writer_append_begin_object(&jw);
  if (az_result_failed(rc))
    ESP_LOGE(TAG, "Telemetry payload failed %d", rc);

  // rc = az_json_writer_append_property_name(&jw,
  //                                         AZ_SPAN_LITERAL_FROM_STR("msgCount"));
  // if (az_result_failed(rc))
  //   ESP_LOGE(TAG, "Telemetry payload failed %d", rc);
  // rc = az_json_writer_append_int32(&jw, ++telemetry_send_count);
  // if (az_result_failed(rc))
  //   ESP_LOGE(TAG, "Telemetry payload failed %d", rc);

  float batteryVoltage = M5.Axp.GetBatVoltage();
  float temperature = 0.0F;
  M5.IMU.getTempData(&temperature);
  ESP_LOGD(TAG, "voltage=%f temperature=%f",
    batteryVoltage, temperature);

  rc = az_json_writer_append_property_name(&jw,
                                          AZ_SPAN_LITERAL_FROM_STR("voltage"));
  if (az_result_failed(rc))
    ESP_LOGE(TAG, "Telemetry payload failed %d", rc);
  rc = az_json_writer_append_double(&jw, batteryVoltage, 3);
  if (az_result_failed(rc))
    ESP_LOGE(TAG, "Telemetry payload failed %d", rc);

  rc = az_json_writer_append_property_name(&jw,
                                          AZ_SPAN_LITERAL_FROM_STR("currentTemperature"));
  if (az_result_failed(rc))
    ESP_LOGE(TAG, "Telemetry payload failed %d", rc);
  rc = az_json_writer_append_double(&jw, temperature, 1);
  if (az_result_failed(rc))
    ESP_LOGE(TAG, "Telemetry payload failed %d", rc);

  rc = az_json_writer_append_end_object(&jw);
  if (az_result_failed(rc))
    ESP_LOGE(TAG, "Telemetry payload failed %d", rc);

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

static void sendTelemetry() {
  az_result rc;
  az_span telemetry = AZ_SPAN_FROM_BUFFER(telemetry_payload);

  uint8_t property_buffer[64];
  az_span property_span = AZ_SPAN_FROM_BUFFER(property_buffer);
  az_iot_message_properties props;
  rc = az_iot_message_properties_init(&props, property_span, 0);
  if (az_result_failed(rc))
    ESP_LOGE(TAG, "Telemetry failed %d", rc);
  rc = az_iot_message_properties_append(
      &props, AZ_SPAN_FROM_STR(AZ_IOT_MESSAGE_PROPERTIES_CONTENT_TYPE),
      AZ_SPAN_LITERAL_FROM_STR("application/json"));
  if (az_result_failed(rc))
    ESP_LOGE(TAG, "Telemetry failed %d", rc);
  rc = az_iot_message_properties_append(
      &props, AZ_SPAN_FROM_STR(AZ_IOT_MESSAGE_PROPERTIES_CONTENT_ENCODING),
      AZ_SPAN_LITERAL_FROM_STR("utf-8"));
  if (az_result_failed(rc))
    ESP_LOGE(TAG, "Telemetry failed %d", rc);

  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to
  // reflect the current values of the properties.
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, &props, telemetry_topic, sizeof(telemetry_topic), NULL))) {
    DemoConsole.writeMessage(
        "ERROR: Failed az_iot_hub_client_telemetry_get_publish_topic");
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

  DemoConsole.writeMessage("Sending telemetry %s", telemetry);

  if (mqtt_client.publish(telemetry_topic, az_span_ptr(telemetry),
                          az_span_size(telemetry), DO_NOT_RETAIN_MSG)) {
    ESP_LOGD(TAG, "Message published successfully");
  } else {
    DemoConsole.writeMessage("ERROR: Failed publishing");
  }
}

// Arduino setup and loop main functions.

void setup() {
  Serial.begin(115200);
  ESP_LOGI(TAG, "** Setup **");

  M5.begin();
  M5.IMU.Init();
  DemoConsole.begin();

  bool missingConfig = false;
  if (ssid == "") {
    DemoConsole.writeMessage("WiFi SSID missing");
    missingConfig = true;
  } else {
    DemoConsole.writeMessage("Using WiFi SSID: %s", ssid);
  }
  if (password == "") {
    DemoConsole.writeMessage("WiFi Password missing");
    missingConfig = true;
  }
  if (host == "") {
    DemoConsole.writeMessage("IoT Hub FQDN missing");
    missingConfig = true;
  } else {
    DemoConsole.writeMessage("Using IoT Hub: %s", host);
  }

#ifndef IOT_CONFIG_USE_X509_CERT
  if (device_key == "") {
    DemoConsole.writeMessage("IoT Hub Key missing");
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

if (config_device_id == "") {
  snprintf(device_id, sizeof(device_id), "eui-%s", StartNetwork.eui64().c_str());
  DemoConsole.writeMessage("Using default Device ID: %s", device_id);
} else {
  snprintf(device_id, sizeof(device_id), "%s", config_device_id);
  DemoConsole.writeMessage("Using configured Device ID: %s", device_id);
}

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
    if (!mqtt_client.connected()) {
      initializeMqttClient();
    }
#ifndef IOT_CONFIG_USE_X509_CERT
    else if (sas_token.IsExpired()) {
      DemoConsole.writeMessage(
          "SAS token expired; reconnecting with a new one");
      //(void)esp_mqtt_client_destroy(mqtt_client);
      mqtt_client.disconnect();
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
