#include "WiFiNetworkService.h"

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
static char eui64_buffer[17];
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
    ESP_LOGD(TAG, "WiFi AP start, enabling IPv6");
    // NOTE: Need a short delay, otherwise failures
    delay(100);
    WiFi.softAPenableIpV6();
    break;
  case ARDUINO_EVENT_WIFI_STA_START:
    ESP_LOGD(TAG, "WiFi station start");
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    ESP_LOGD(TAG, "WiFi station connected, enabling IPv6");
    // NOTE: Need a short delay, otherwise failures
    delay(100);
    WiFi.enableIpV6();
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    _logger->information("WiFi station IPv6 " IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
    ip6_address_type = esp_netif_ip6_get_addr_type(&info.got_ip6.ip6_info.ip);
    ESP_LOGD(TAG, "IPv6 address type %d", ip6_address_type);
    if (ip6_address_type == ESP_IP6_ADDR_IS_GLOBAL) {
      delay(100);
      _logger->success();
    }
    break;
  case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
    _logger->information("WiFi AP IPv6 " IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    delay(100);
    status_ = CONNECTED;
    _logger->information("WiFi station connected IPv4 %s", WiFi.localIP().toString().c_str());
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

WiFiNetworkService::WiFiNetworkService() {
}

void WiFiNetworkService::begin() {
  _logger->information("Begin WiFi Network Manager, SSID: <%s>", ssid_);
  //ESP_LOGV(TAG, "WiFi password <%s>", password_);
  _logger->pending();
  WiFi.onEvent(wifiOnEvent);
}

const char * WiFiNetworkService::eui64() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  // 01:34:67:9A:CD:F0
  // 0<3>3467 fffe 9acdf0
  snprintf(eui64_buffer, sizeof(eui64_buffer), "%02x%02x%02xfffe%02x%02x%02x", mac[0] ^ 2, mac[1], mac[2], mac[3], mac[4], mac[5]);
  return eui64_buffer;
}

IPv6Address WiFiNetworkService::globalIPv6(){
	esp_ip6_addr_t addr;
  if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
    return IPv6Address();
  }
  if(esp_netif_get_ip6_global(get_esp_interface_netif(ESP_IF_WIFI_STA), &addr)) {
    return IPv6Address();
  }
  return IPv6Address(addr.addr);
}

bool WiFiNetworkService::isConnected() { return status_ == CONNECTED; }

void WiFiNetworkService::loop() {
  if (status_ == NOT_CONNECTED) {
    unsigned long now = millis();
    if (now > retry_at_millis) {
      status_ = CONNECTING;
      WiFi.disconnect(true);
      WiFi.mode(WIFI_MODE_STA);
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

String WiFiNetworkService::mainDnsIP(){
	esp_netif_dns_info_t dns;
  if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
    return "";
  }
  if(esp_netif_get_dns_info(get_esp_interface_netif(ESP_IF_WIFI_STA), ESP_NETIF_DNS_MAIN, &dns)) {
    return "ERROR";
  }
  if(dns.ip.type == ESP_IPADDR_TYPE_V6) {
    return IPv6Address(dns.ip.u_addr.ip6.addr).toString();
  } else {
    return IPAddress(dns.ip.u_addr.ip4.addr).toString();
  }
}

void WiFiNetworkService::setCredentials(const char *ap_password, const char *ssid, const char *password) {
    ap_password_ = ap_password;
    ssid_ = ssid;
    password_ = password;
}

void WiFiNetworkService::setEventLogger(EventLogger *eventLogger) {
    _logger = eventLogger;
}
