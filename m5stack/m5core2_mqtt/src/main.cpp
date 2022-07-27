#include <Arduino.h>
#include <M5Core2.h>
//#include <WiFi.h>
#include <WiFiClientSecure.h>
//#include <WiFiMulti.h>
#include <PubSubClient.h>

#include "DemoConsole.h"
#include "StartNetwork.h"
#include "config.h"

#include "esp_log.h"
static const char* TAG = "demo";

static const char* ssid = WIFI_SSID;
static const char* password = WIFI_PASSWORD;
static const char* mqttServer = MQTT_SERVER;
static const int mqttPort = MQTT_PORT;
static const char* mqttUser = MQTT_USER;
static const char* mqttPassword = MQTT_PASSWORD;

extern const uint8_t root_ca_pem_start[] asm("_binary_src_certs_ISRG_Root_X1_pem_start");
extern const uint8_t root_ca_pem_end[] asm("_binary_src_certs_ISRG_Root_X1_pem_end");

#define MESSAGE_BUFFER_SIZE (100)
#define SEND_INTERVAL_MS (5000)

unsigned long nextMessageMilliseconds = 0;
char message[MESSAGE_BUFFER_SIZE];
static const char* messageTemplate = "[{\"n\":\"urn:dev:mac:%s\",\"u\":\"Cel\",\"v\":%d}]";
int value;

WiFiClientSecure *wifiClientSecure = NULL;
PubSubClient pubSubClient;

RTC_DateTypeDef rtcDateNow;
RTC_TimeTypeDef rtcTimeNow;

void callback(char* topic, byte* payload, unsigned int length) {
  DemoConsole.print("Message arrived [");
  DemoConsole.print(topic);
  DemoConsole.print("] ");
  for (int i = 0; i < length; i++) {
    DemoConsole.printf("%c", payload[i]);
  }
  DemoConsole.print("\n");
}

void reConnect() {
  if (!pubSubClient.connected()) {
    DemoConsole.print("Attempting MQTT connection...");
    if (wifiClientSecure) {
      ESP_LOGD(TAG, "Deleting wifClientSecure");
      delete wifiClientSecure;
    }
    wifiClientSecure = new WiFiClientSecure;
    if (wifiClientSecure) {
      ESP_LOGD(TAG, "Created new wifiClientSecure");

      wifiClientSecure->setCACert((char *)root_ca_pem_start);

      pubSubClient.setClient(*wifiClientSecure);

      // Create a random client ID.  创建一个随机的客户端ID
      char clientId[21];
      snprintf(clientId, sizeof(clientId), "eui-%s", StartNetwork.eui64());
      if (pubSubClient.connect(clientId, mqttUser, mqttPassword)) {
        DemoConsole.printf("\nSuccess\n");
        // Once connected, publish an announcement to the topic.  一旦连接，发送一条消息至指定话题
        pubSubClient.publish("test", "{\"m\"=\"client_connected\"}");
        // ... and resubscribe.  重新订阅话题
        pubSubClient.subscribe("test");
      } else {
        DemoConsole.print("failed, rc=");
        DemoConsole.printf("%d", pubSubClient.state());
        DemoConsole.print(", con=");
        DemoConsole.printf("%d", pubSubClient.connected());
        DemoConsole.print("\n");
      }
    } else {
      DemoConsole.print("Unable to create client\n");
    }
  }
}

void wifiConnectedLoop(){
  if (!pubSubClient.connected()) {
    reConnect();
  }
  delay(100);
  if (pubSubClient.connected()) {
    pubSubClient.loop();

    unsigned long nowMilliseconds = millis();
    if (nowMilliseconds > nextMessageMilliseconds) {
      nextMessageMilliseconds = nowMilliseconds + SEND_INTERVAL_MS;
      ++value;
      snprintf(message, MESSAGE_BUFFER_SIZE, messageTemplate, StartNetwork.eui64(), value);
      DemoConsole.print("Publish:");
      DemoConsole.print(message);
      DemoConsole.print("\n");
      pubSubClient.publish("test", message);
    }
  }
}

void setup() {
  M5.begin();
  Serial.begin(115200);
  ESP_LOGI(TAG, "** Setup **");

  DemoConsole.begin();

  DemoConsole.printf("Connecting to %s", ssid);
  DemoConsole.print("\n");

  if (ssid=="") {
    DemoConsole.print("SSID missing\n");
    return;
  }
  if (password=="") {
    DemoConsole.print("Password missing\n");
    return;
  }

  StartNetwork.begin(ssid, password);

  pubSubClient.setServer(mqttServer, mqttPort);
  //IPAddress mqttIp = IPAddress(20,213,94,9);
  //client.setServer(mqttIp, mqttPort);
  pubSubClient.setCallback(callback); 

  delay(1000);
}

void loop() {
  ESP_LOGD(TAG, "** Loop %d **", millis());

  M5.update();
  DemoConsole.loop();

  if(StartNetwork.wifiConnected()){
    wifiConnectedLoop();
  }

  delay(1000);
}
