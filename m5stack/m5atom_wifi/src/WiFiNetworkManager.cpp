#include "WiFiNetworkManager.h"

#define REQUIRE_ONE_SET_SSID_PW       true
//#define USE_LITTLEFS          false
//#define USE_SPIFFS            true

#include <ESPAsync_WiFiManager_Lite.h>
#include <WiFi.h>
#include <Arduino.h>
#include <esp_log.h>

typedef enum {
  NOT_CONNECTED = 0,
  CONFIG_MODE = 1,
  CONNECTED = 2
} NetworkStatus;

static const char *TAG = "Network";

const char *ap_password_ = { 0 };
const char *ssid_ = { 0 };
const char *password_ = { 0 };
uint16_t retry_count = 0;
unsigned long retry_at_millis = 0;
NetworkStatus status_ = NOT_CONNECTED;
ESPAsync_WiFiManager_Lite* wifi_manager;

void wifiOnEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
  case ARDUINO_EVENT_WIFI_AP_START:
    ESP_LOGD(TAG, "WiFi AP start, enabling IPv6");
    WiFi.softAPenableIpV6();
    break;
  case ARDUINO_EVENT_WIFI_STA_START:
    ESP_LOGD(TAG, "WiFi station start");
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    ESP_LOGD(TAG, "WiFi station connected, enabling IPv6");
    WiFi.enableIpV6();
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    ESP_LOGD(TAG, "WiFi station IPv6 " IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
    break;
  case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
    ESP_LOGD(TAG, "WiFi AP IPv6 " IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    ESP_LOGD(TAG, "WiFi station IPv4 %s", WiFi.localIP().toString().c_str());
    //connected_ = true;
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    ESP_LOGD(TAG, "WiFi station disconnected");
    //connected_ = false;
    break;
  default:
    break;
  }
}

WiFiNetworkManager::WiFiNetworkManager() {
}

void WiFiNetworkManager::begin() {
  eventLogger()->information("Begin WiFi Network Manager, SSID: %s", ssid_);
  eventLogger()->pending();
  WiFi.onEvent(wifiOnEvent);

  wifi_manager = new ESPAsync_WiFiManager_Lite();
  wifi_manager->begin();
}

bool WiFiNetworkManager::isConnected() { return status_ == CONNECTED; }

void WiFiNetworkManager::loop() {
  wifi_manager->run();

  if (status_ != CONNECTED) {
    NetworkStatus new_status;
    if (WiFi.status() == WL_CONNECTED) {
      new_status = CONNECTED;
    } else if (wifi_manager->isConfigMode()) {
      new_status = CONFIG_MODE;
    }
    if (new_status != status_) {
      status_ = new_status;
      if (status_ == CONNECTED) {
        eventLogger()->information("Connected");
        eventLogger()->ready();
      }
    }
  }

  /*
  if (!connected_) {
    unsigned long now = millis();
    if (now > retry_at_millis) {
      wifi_manager = new ESPAsync_WiFiManager_Lite();
      wifi_manager->begin()

      uint8_t mac[6];
      WiFi.macAddress(mac);
      char ap_name[9] = {0};
      snprintf(ap_name, sizeof(eui64_buffer), "ap%02x%02x%02x",
           mac[3], mac[4], mac[5]);

      WiFiManager wm;
      //wm.resetSettings();
      bool success = wm.autoConnect(ap_name, ap_password_);
      if (success) {
        connected_ = true;
        eventLogger()->information("Connected");
        eventLogger()->ready();
      } else {
        retry_count++;
        eventLogger()->error("Connection failed %d", retry_count);
        retry_at_millis = millis() + 1000;
      }
    }
  }*/

}

void WiFiNetworkManager::setCredentials(const char *ap_password, const char *ssid, const char *password) {
    ap_password_ = ap_password;
    ssid_ = ssid;
    password_ = password;
}
