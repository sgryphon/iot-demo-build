#include <Arduino.h>
#include <M5Core2.h>
#include <WiFi.h>
//#include <WiFiMulti.h>
#include <PubSubClient.h>

#include "StartNetwork.h"
#include "config.h"

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
static const char* messageTemplate = "[{\"t\":%d,\"n\":\"urn:dev:mac:%s\",\"u\":\"Cel\",\"v\":%d}]";
int value;

WiFiClient espClient;
PubSubClient client(espClient);

RTC_DateTypeDef rtcDateNow;
RTC_TimeTypeDef rtcTimeNow;

void callback(char* topic, byte* payload, unsigned int length) {
  M5.Lcd.print("Message arrived [");
  M5.Lcd.print(topic);
  M5.Lcd.print("] ");
  for (int i = 0; i < length; i++) {
    M5.Lcd.print((char)payload[i]);
  }
  M5.Lcd.println();
}

void reConnect() {
  if (!client.connected()) {
    M5.Lcd.print("Attempting MQTT connection...");
    // Create a random client ID.  创建一个随机的客户端ID
    String clientId = "M5Stack-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect.  尝试重新连接
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      M5.Lcd.printf("\nSuccess\n");
      // Once connected, publish an announcement to the topic.  一旦连接，发送一条消息至指定话题
      client.publish("M5Stack", "hello world");
      // ... and resubscribe.  重新订阅话题
      client.subscribe("M5Stack");
    } else {
      M5.Lcd.print("failed, rc=");
      M5.Lcd.print(client.state());
      M5.Lcd.print(", con=");
      M5.Lcd.print(client.connected());
      M5.Lcd.print("\n");
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

void printWiFi() {
  M5.Rtc.GetDate(&rtcDateNow);
  M5.Rtc.GetTime(&rtcTimeNow);

  M5.Lcd.setCursor(0, 0);

  M5.Lcd.printf("Clock %04d-%02d-%02d %02d:%02d:%02d",
    rtcDateNow.Year, rtcDateNow.Month, rtcDateNow.Date,
    rtcTimeNow.Hours, rtcTimeNow.Minutes, rtcTimeNow.Seconds);
  M5.Lcd.print("\n");

  M5.Lcd.printf("WiFi MAC: ");
  M5.Lcd.printf(WiFi.macAddress().c_str());
  M5.Lcd.printf(" Status: %3d", WiFi.status());
  M5.Lcd.print("\n");

  M5.Lcd.print("IPv6: ");
  M5.Lcd.print(StartNetwork.globalIPv6());
  M5.Lcd.print("\n");

  M5.Lcd.print("IPv4: ");
  M5.Lcd.print(WiFi.localIP());
  M5.Lcd.print("\n");

  int16_t x = M5.Lcd.getCursorX();
  int16_t y = M5.Lcd.getCursorY();
  M5.Lcd.fillRect(x, y, 320 - x, 240 - y, 0);
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
      ++value;
      String eui64 = macToEui64(WiFi.macAddress());
      snprintf(message, MESSAGE_BUFFER_SIZE, "[{\"t\":%d,\"n\":\"urn:dev:ow:%s\",\"u\":\"Cel\",\"v\":%d}]", nowMilliseconds, eui64.c_str(), value);
      M5.Lcd.print("Publish");
      M5.Lcd.print(message);
      M5.Lcd.print("\n");
      client.publish("M5Stack", message);
      nextMessageMilliseconds = nowMilliseconds + SEND_INTERVAL_MS;
    }
  }
}

void setup() {
  M5.begin();

  printWiFi();

  M5.Lcd.printf("Connecting to %s", ssid);
  M5.Lcd.print("\n");

  if (ssid=="") {
    M5.Lcd.print("SSID missing");
    M5.Lcd.print("\n");
    return;
  }
  if (password=="") {
    M5.Lcd.print("Password missing");
    M5.Lcd.print("\n");
    return;
  }

  Serial.begin(115200);
  Serial.println("STA Connecting");

  StartNetwork.begin(ssid, password);

  //client.setServer(mqttServer, mqttPort);
  IPAddress mqttIp = IPAddress(20,213,94,9);
  client.setServer(mqttIp, mqttPort);
  client.setCallback(callback); 

  delay(1000);
}

void loop() {
  M5.update();

  if(StartNetwork.wifiConnected()){
    wifiConnectedLoop();
  }

  if (M5.Lcd.getCursorY() > 220) {
    printWiFi();
  }

  delay(1000);
}
