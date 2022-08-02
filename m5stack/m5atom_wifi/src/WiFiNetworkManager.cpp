#include "WiFiNetworkManager.h"

#include <WiFi.h>
#include <Arduino.h>
#include <esp_log.h>

typedef enum {
  NOT_CONNECTED = 0,
  CONNECTING = 1,
  CONNECTED = 2
} NetworkStatus;

static const char *TAG = "Network";

const char *ap_password_ = { 0 };
EventLogger *eventLogger_ = nullptr;
const char *ssid_ = { 0 };
const char *password_ = { 0 };
uint16_t retry_count = 0;
unsigned long retry_at_millis = 0;
NetworkStatus status_ = NOT_CONNECTED;

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
    status_ = CONNECTED;
    eventLogger_->information("Connected");
    eventLogger_->ready();
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    ESP_LOGD(TAG, "WiFi station disconnected");
//    retry_count++;
//    eventLogger_->error("Connection failed %d", retry_count);
//    status_ = NOT_CONNECTED;
    break;
  default:
    break;
  }
}

WiFiNetworkManager::WiFiNetworkManager() {
}

void WiFiNetworkManager::begin() {
  eventLogger_->information("Begin WiFi Network Manager, SSID: %s", ssid_);
  eventLogger_->pending();
  WiFi.onEvent(wifiOnEvent);
}

bool WiFiNetworkManager::isConnected() { return status_ == CONNECTED; }

void WiFiNetworkManager::loop() {
  if (status_ == NOT_CONNECTED) {
    unsigned long now = millis();
    if (now > retry_at_millis) {
      status_ = CONNECTING;
      WiFi.disconnect(true);
      WiFi.mode(WIFI_MODE_STA);
      WiFi.begin(ssid_, password_);
      /*
      if (success) {
        connected_ = true;
        eventLogger()->information("Connected");
        eventLogger()->ready();
      } else {
        retry_count++;
        eventLogger()->error("Connection failed %d", retry_count);
        retry_at_millis = millis() + 1000;
      }
      */
    }
  }
}

void WiFiNetworkManager::setCredentials(const char *ap_password, const char *ssid, const char *password) {
    ap_password_ = ap_password;
    ssid_ = ssid;
    password_ = password;
}

void WiFiNetworkManager::setEventLogger(EventLogger *eventLogger) {
    eventLogger_ = eventLogger;
}
