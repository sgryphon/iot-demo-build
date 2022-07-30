#include "StartNetwork.h"
#include "DemoConsole.h"

#include <M5Core2.h>
#include <WiFi.h>
#include <esp_log.h>

static const char *TAG = "demo";

#define START_NETWORK_MD5_LCD 1

static const char *wifi_ssid = NULL;
static const char *wifi_password = NULL;

#define AP_SSID "esp32-v6"

static volatile bool wifi_connected = false;

esp_netif_t *get_esp_interface_netif(esp_interface_t interface);

void wifiOnGotIPv4() {
#ifdef START_NETWORK_MD5_LCD
  DemoConsole.writeMessage("WiFi address IPv4 %s",
                           WiFi.localIP().toString().c_str());
#endif
  ESP_LOGI(TAG, "LOG: WiFi connected. IPv4 %s",
           WiFi.localIP().toString().c_str());
}

void wifiOnDisconnect() {
#ifdef START_NETWORK_MD5_LCD
  DemoConsole.writeMessage("WARNING: WiFi disconnected");
#endif
  ESP_LOGW(TAG, "WiFi disconnected: waiting 1000ms then reconnect");
  delay(1000);
  WiFi.begin(wifi_ssid, wifi_password);
}

void wifiOnEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
  case ARDUINO_EVENT_WIFI_AP_START:
    // can set ap hostname here
    WiFi.softAPsetHostname(AP_SSID);
    // enable ap ipv6 here
    WiFi.softAPenableIpV6();
    break;
  case ARDUINO_EVENT_WIFI_STA_START:
    // set sta hostname here
    WiFi.setHostname(AP_SSID);
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    // enable sta ipv6 here
    //  NOTE: Need a short delay (e.g. printing a message, or explicit delay)
#ifdef START_NETWORK_MD5_LCD
    DemoConsole.writeMessage("WiFi connected, enabling IPv6");
#endif
    ESP_LOGD(TAG, "LOG: WiFi connected, enabling IPv6");
    delay(100);
    WiFi.enableIpV6();
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    // char ipv6[40];
    // snprintf(ipv6, sizeof(ipv6), IPV6STR,
    // IPV62STR(info.got_ip6.ip6_info.ip));
#ifdef START_NETWORK_MD5_LCD
    DemoConsole.writeMessage("WiFi IPv6 " IPV6STR,
                             IPV62STR(info.got_ip6.ip6_info.ip));
#endif
    ESP_LOGD(TAG, "LOG: WiFi IP6 " IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
    break;
  case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
    ESP_LOGD(TAG, "LOG: AP IP6 " IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    wifiOnGotIPv4();
    wifi_connected = true;
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    wifi_connected = false;
    wifiOnDisconnect();
    break;
  default:
    break;
  }
}

void StartNetworkClass::begin(const char *ssid, const char *password) {
  // TODO: Should copy these strings, but we know they are referencing constants
  wifi_ssid = ssid;
  wifi_password = password;
  WiFi.disconnect(true);
  WiFi.onEvent(wifiOnEvent);
  // WiFi.mode(WIFI_MODE_APSTA);
  // WiFi.softAP(AP_SSID);
  // WiFi.begin(STA_SSID, STA_PASS);
  WiFi.begin(wifi_ssid, wifi_password);
};

String StartNetworkClass::eui64() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  // 01:34:67:9A:CD:F0
  // 0<3>3467 fffe 9acdf0
  char eui64_buffer[17] = {0};
  snprintf(eui64_buffer, sizeof(eui64_buffer), "%02x%02x%02xfffe%02x%02x%02x",
           mac[0] ^ 2, mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(eui64_buffer);
}

IPv6Address StartNetworkClass::globalIPv6() {
  esp_ip6_addr_t addr;
  if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
    return IPv6Address();
  }
  if (esp_netif_get_ip6_global(get_esp_interface_netif(ESP_IF_WIFI_STA),
                               &addr)) {
    return IPv6Address();
  }
  return IPv6Address(addr.addr);
}

String StartNetworkClass::mainDnsIP() {
  esp_netif_dns_info_t dns;
  if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
    return "";
  }
  if (esp_netif_get_dns_info(get_esp_interface_netif(ESP_IF_WIFI_STA),
                             ESP_NETIF_DNS_MAIN, &dns)) {
    return "";
  }
  if (dns.ip.type == ESP_IPADDR_TYPE_V6) {
    return IPv6Address(dns.ip.u_addr.ip6.addr).toString();
  } else {
    return IPAddress(dns.ip.u_addr.ip4.addr).toString();
  }
}

bool StartNetworkClass::wifiConnected() { return wifi_connected; };

StartNetworkClass StartNetwork;