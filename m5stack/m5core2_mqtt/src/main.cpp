#include <Arduino.h>
#include <M5Core2.h>
#include <WiFi.h>
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

#define MESSAGE_BUFFER_SIZE (100)
#define SEND_INTERVAL_MS (5000)

unsigned long nextMessageMilliseconds = 0;
char message[MESSAGE_BUFFER_SIZE];
static const char* messageTemplate = "[{\"n\":\"urn:dev:mac:%s\",\"u\":\"Cel\",\"v\":%d}]";
int value;

WiFiClient espClient;
PubSubClient client(espClient);

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
  if (!client.connected()) {
    DemoConsole.print("Attempting MQTT connection...");
    // Create a random client ID.  创建一个随机的客户端ID
    String clientId = "M5Stack-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect.  尝试重新连接
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      DemoConsole.printf("\nSuccess\n");
      // Once connected, publish an announcement to the topic.  一旦连接，发送一条消息至指定话题
      client.publish("M5Stack", "hello world");
      // ... and resubscribe.  重新订阅话题
      client.subscribe("M5Stack");
    } else {
      DemoConsole.print("failed, rc=");
      DemoConsole.printf("%d", client.state());
      DemoConsole.print(", con=");
      DemoConsole.printf("%d", client.connected());
      DemoConsole.print("\n");
    }
  }
}

// 01:34:67:9a:cd:f0
// 0<3>3467 fffe 9acdf0
String macToEui64(String mac) {
  byte n = (mac[1] < '9') ? mac[1] - '0' : mac[1] - '7';
  n = n ^ 2;  
  return mac.substring(0,1) + "2" + mac.substring(3,2) + mac.substring(6,2)
    + "fffe" + mac.substring(9,2) + mac.substring(12,2) + mac.substring(15,2);
}

void wifiConnectedLoop(){
  if (!client.connected()) {
    reConnect();
  }
  delay(100);
  if (client.connected()) {
    client.loop();

    unsigned long nowMilliseconds = millis();
    if (nowMilliseconds > nextMessageMilliseconds) {
      nextMessageMilliseconds = nowMilliseconds + SEND_INTERVAL_MS;
      ++value;
      String eui64 = macToEui64(WiFi.macAddress());
      snprintf(message, MESSAGE_BUFFER_SIZE, messageTemplate, eui64.c_str(), value);
      DemoConsole.print("Publish:");
      DemoConsole.print(message);
      DemoConsole.print("\n");
      client.publish("M5Stack", message);
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

  client.setServer(mqttServer, mqttPort);
  //IPAddress mqttIp = IPAddress(20,213,94,9);
  //client.setServer(mqttIp, mqttPort);
  client.setCallback(callback); 

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
