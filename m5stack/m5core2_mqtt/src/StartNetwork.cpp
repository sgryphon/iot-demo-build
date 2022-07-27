#include "StartNetwork.h"
#include <M5Core2.h>
#include <WiFi.h>
#include "DemoConsole.h"
#include "esp_log.h"
static const char* TAG = "demo";

#define START_NETWORK_MD5_LCD 1

static const char* _ssid = NULL;
static const char* _password = NULL;

//#define STA_SSID "**********"
//#define STA_PASS "**********"
#define AP_SSID  "esp32-v6"

static volatile bool wifi_connected = false;

esp_netif_t* get_esp_interface_netif(esp_interface_t interface);

void wifiOnConnect(){
#ifdef START_NETWORK_MD5_LCD
  DemoConsole.print("STA Connected\n");
  DemoConsole.print("STA IPv4: ");
  DemoConsole.print(WiFi.localIP());
  DemoConsole.print("\n");
#endif
  ESP_LOGI(TAG, "StartNetwork wifiOnConnect: IPv4 %s", WiFi.localIP().toString().c_str());

  //ntpClient.begin(2390);
}

void wifiOnDisconnect(){
  ESP_LOGI(TAG, "StartNetwork wifiOnDisconnect: waiting 1000ms then reconnect");
#ifdef START_NETWORK_MD5_LCD
  DemoConsole.print("STA Disconnected\n");
#endif
  delay(1000);
  WiFi.begin(_ssid, _password);
}

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info){
    switch(event) {
        case ARDUINO_EVENT_WIFI_AP_START:
            //can set ap hostname here
            WiFi.softAPsetHostname(AP_SSID);
            //enable ap ipv6 here
            WiFi.softAPenableIpV6();
            break;

        case ARDUINO_EVENT_WIFI_STA_START:
            //set sta hostname here
            WiFi.setHostname(AP_SSID);
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            //enable sta ipv6 here
            // NOTE: Need a short delay (e.g. printing a message, or explicit delay)
            ESP_LOGD(TAG, "WIFI_STA_CONNECTED: enabling IPv6");
            delay(100);
            WiFi.enableIpV6();
#ifdef START_NETWORK_MD5_LCD
            DemoConsole.print("STA_CONNECTED, ** enable IPv6 **\n");
#endif
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            //char ipv6[40];
            //snprintf(ipv6, sizeof(ipv6), IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
            ESP_LOGD(TAG, "WIFI STA_GOT_IP6: " IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
#ifdef START_NETWORK_MD5_LCD
            DemoConsole.print("STA IPv6: ");
            DemoConsole.printf(IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
            //M5.Lcd.print(WiFi.localIPv6());
            DemoConsole.print("\n");
#endif
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
#ifdef START_NETWORK_MD5_LCD
            DemoConsole.print("AP IPv6: ");
            DemoConsole.print(WiFi.softAPIPv6());
            DemoConsole.print("\n");
#endif
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            wifiOnConnect();
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

void StartNetworkClass::begin(const char* ssid, const char* password)
{
  // TODO: Should copy these strings, but we know they are referencing constants
  _ssid = ssid;
  _password = password;
  WiFi.disconnect(true);
  WiFi.onEvent(WiFiEvent);
  //WiFi.mode(WIFI_MODE_APSTA);
  //WiFi.softAP(AP_SSID);
  //WiFi.begin(STA_SSID, STA_PASS);
  WiFi.begin(_ssid, _password);
};

IPv6Address StartNetworkClass::globalIPv6(){
	esp_ip6_addr_t addr;
  if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
    return IPv6Address();
  }
  if(esp_netif_get_ip6_global(get_esp_interface_netif(ESP_IF_WIFI_STA), &addr)) {
    return IPv6Address();
  }
  return IPv6Address(addr.addr);
}

String StartNetworkClass::mainDnsIP(){
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

bool StartNetworkClass::wifiConnected()
{
  return wifi_connected;
};

StartNetworkClass StartNetwork;