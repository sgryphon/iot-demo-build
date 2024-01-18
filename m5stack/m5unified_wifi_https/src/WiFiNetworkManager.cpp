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
EventLogger *_logger = nullptr;
const char *ssid_ = { 0 };
const char *password_ = { 0 };
uint16_t retry_count = 0;
unsigned long retry_at_millis = 0;
NetworkStatus status_ = NOT_CONNECTED;

esp_netif_t *get_esp_interface_netif(esp_interface_t interface);

void wifiOnEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  esp_ip6_addr_type_t ip6_address_type;
  switch (event) {
  case ARDUINO_EVENT_WIFI_AP_START:
    ESP_LOGD(TAG, "WiFi AP start");
    //can set ap hostname here
    //WiFi.softAPsetHostname(AP_SSID);
    break;
  case ARDUINO_EVENT_WIFI_STA_START:
    ESP_LOGD(TAG, "WiFi station start");
    //set sta hostname here
    //WiFi.setHostname(AP_SSID);
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    _logger->information("WF STA connected");
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    _logger->information("WF STA IPv6 " IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
    ip6_address_type = esp_netif_ip6_get_addr_type(&info.got_ip6.ip6_info.ip);
    if (ip6_address_type == ESP_IP6_ADDR_IS_GLOBAL || ip6_address_type == ESP_IP6_ADDR_IS_UNIQUE_LOCAL) {
      _logger->success();
    }
    ESP_LOGD(TAG, "IPv6 address type %d", ip6_address_type);
    break;
  case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
    _logger->information("WF AP IPv6 " IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    delay(100);
    status_ = CONNECTED;
    _logger->information("WF STA IPv4 %s", WiFi.localIP().toString().c_str());
    _logger->success();
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    _logger->error("WiFi station disconnected");
    delay(100);
//    retry_count++;
//    _logger->error("Connection failed %d", retry_count);
//    status_ = NOT_CONNECTED;
//    retry_at_millis = millis() + 1000;
    break;
  default:
    break;
  }
}

WiFiNetworkManager::WiFiNetworkManager() {
}

void WiFiNetworkManager::begin() {
  _logger->information("Begin WiFi Network Manager, SSID: <%s>", ssid_);
  _logger->pending();
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
      WiFi.enableIPv6();
      // WiFi.mode(WIFI_MODE_APSTA);
      // WiFi.softAPenableIPv6();
      // WiFi.softAP(AP_SSID);
      wl_status_t rc = WiFi.begin(ssid_, password_);
      ESP_LOGD(TAG, "WiFi begin status %d", rc);
      switch (rc) {
        case WL_NO_SSID_AVAIL:
          _logger->error("No SSID available");
          break;
      }

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
    _logger = eventLogger;
}
