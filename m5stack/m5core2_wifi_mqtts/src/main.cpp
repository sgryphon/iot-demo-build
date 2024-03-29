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
static const int mqtt_port = MQTT_PORT;
static const char* mqttUser = MQTT_USER;
static const char* mqttPassword = MQTT_PASSWORD;

extern const uint8_t root_ca_pem_start[] asm("_binary_src_certs_ISRG_Root_X1_pem_start");
extern const uint8_t root_ca_pem_end[] asm("_binary_src_certs_ISRG_Root_X1_pem_end");

#define MESSAGE_BUFFER_SIZE (255)
#define SEND_INTERVAL_MS (5000)

unsigned long nextMessageMilliseconds = 0;
char message[MESSAGE_BUFFER_SIZE];

int counter = 0;

// generic
const char* messageTemplate0 = \
  "{\"battery_volts\":%.3f," \
  "\"battery_amps\":%.3f," \
  "\"battery_watts\":%.1f," \
  "\"temperature_celsius\":%.2f}";

// SenML message
const char* messageTemplate1 = \
  "[{\"bn\":\"urn:dev:mac:%s_\",\"n\":\"voltage\",\"u\":\"V\",\"v\":%.3f}," \
  "{\"n\":\"current\",\"u\":\"A\",\"v\":%.3f}," \
  "{\"n\":\"power\",\"u\":\"W\",\"v\":%.1f}," \
  "{\"n\":\"temperature\",\"u\":\"Cel\",\"v\":%.2f}]";

// LwM2M SenML
const char* messageTemplate2 = \
  "[{\"n\":\"/3/0/7/0\",\"v\":%.0f}," \
  "{\"n\":\"/3/0/8/0\",\"v\":%.0f}," \
  "{\"n\":\"/3303/0/5700\",\"v\":%.2f}]";

WiFiClientSecure *wifi_client_secure = NULL;
PubSubClient mqtt_client;

RTC_DateTypeDef rtcDateNow;
RTC_TimeTypeDef rtcTimeNow;

void callback(char* topic, byte* payload, unsigned int length) {
  DemoConsole.print("Message arrived [");
  DemoConsole.print(topic);
  DemoConsole.print("] ");
  for (int i = 0; i < length; i++) {
    DemoConsole.writeMessage("%c", payload[i]);
  }
  DemoConsole.print("\n");
}

void reConnect() {
  if (!mqtt_client.connected()) {
    DemoConsole.print("Attempting MQTT connection...");
    if (wifi_client_secure) {
      ESP_LOGD(TAG, "Deleting wifClientSecure");
      delete wifi_client_secure;
    }
    wifi_client_secure = new WiFiClientSecure;
    if (wifi_client_secure) {
      ESP_LOGD(TAG, "Created new wifiClientSecure");

      wifi_client_secure->setCACert((char *)root_ca_pem_start);

      mqtt_client.setClient(*wifi_client_secure);

      // Create a random client ID.  创建一个随机的客户端ID
      char clientId[21];
      snprintf(clientId, sizeof(clientId), "eui-%s", StartNetwork.eui64());
      if (mqtt_client.connect(clientId, mqttUser, mqttPassword)) {
        DemoConsole.writeMessage("\nSuccess\n");
        // Once connected, publish an announcement to the topic.  一旦连接，发送一条消息至指定话题
        mqtt_client.publish("test", "{\"m\"=\"client_connected\"}");
        // ... and resubscribe.  重新订阅话题
        mqtt_client.subscribe("test");
      } else {
        DemoConsole.print("failed, rc=");
        DemoConsole.writeMessage("%d", mqtt_client.state());
        DemoConsole.print(", con=");
        DemoConsole.writeMessage("%d", mqtt_client.connected());
        DemoConsole.print("\n");
      }
    } else {
      DemoConsole.print("Unable to create client\n");
    }
  }
}

void wifiConnectedLoop(){
  if (!mqtt_client.connected()) {
    reConnect();
  }
  delay(100);
  if (mqtt_client.connected()) {
    mqtt_client.loop();

    unsigned long nowMilliseconds = millis();
    if (nowMilliseconds > nextMessageMilliseconds) {
      nextMessageMilliseconds = nowMilliseconds + SEND_INTERVAL_MS;

      // Build message
      float batteryVoltage = M5.Axp.GetBatVoltage();
      float batteryCurrent = M5.Axp.GetBatCurrent();
      float batteryPower = M5.Axp.GetBatPower();
      bool batteryIsCharging = M5.Axp.isCharging();
      float temperature = 0.0F;
      M5.IMU.getTempData(&temperature);

      ESP_LOGD(TAG, "voltage=%f current=%f power=%f temperature=%f",
        batteryVoltage, batteryCurrent, batteryPower, temperature);

      counter++;
      switch (counter % 3) {
        case 1:
          snprintf(message, MESSAGE_BUFFER_SIZE, messageTemplate1, 
            StartNetwork.eui64(), batteryVoltage, batteryCurrent, batteryPower, temperature);
          break;
        case 2:
          snprintf(message, MESSAGE_BUFFER_SIZE, messageTemplate2, 
            batteryVoltage * 1000, batteryCurrent * 1000, temperature);
          break;
        default:
          snprintf(message, MESSAGE_BUFFER_SIZE, messageTemplate0, 
            batteryVoltage, batteryCurrent, batteryPower, temperature);
          break;
      }

      DemoConsole.print("Publish:");
      DemoConsole.print(message);
      DemoConsole.print("\n");
      mqtt_client.publish("test", message);
    }
  }
}

void setup() {
  Serial.begin(115200);
  ESP_LOGI(TAG, "** Setup **");

  M5.begin();
  M5.IMU.Init();

  DemoConsole.begin();

  DemoConsole.writeMessage("Connecting to %s", ssid);
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

  mqtt_client.setServer(mqttServer, mqtt_port);
  //IPAddress mqttIp = IPAddress(20,213,94,9);
  //client.setServer(mqttIp, mqttPort);
  mqtt_client.setCallback(callback); 

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
