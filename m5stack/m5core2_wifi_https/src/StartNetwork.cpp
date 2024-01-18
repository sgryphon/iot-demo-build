#include "StartNetwork.h"
#include <M5Unified.h>
#include <WiFi.h>
#include "DemoConsole.h"
#include "esp_log.h"
static const char* TAG = "demo";

#define START_NETWORK_MD5_LCD 1

static const char* wifi_ssid = NULL;
static const char* wifi_password = NULL;
static char eui64_buffer[17];

//#define STA_SSID "**********"
//#define STA_PASS "**********"
#define AP_SSID  "esp32-v6"

static volatile bool wifi_connected = false;

esp_netif_t* get_esp_interface_netif(esp_interface_t interface);

void wifiOnGotIPv4(){
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
  WiFi.begin(wifi_ssid, wifi_password);
}

void wifiOnEvent(WiFiEvent_t event, WiFiEventInfo_t info){
    switch(event) {
        case ARDUINO_EVENT_WIFI_AP_START:
            //can set ap hostname here
            WiFi.softAPsetHostname(AP_SSID);
            break;

        case ARDUINO_EVENT_WIFI_STA_START:
            //set sta hostname here
            WiFi.setHostname(AP_SSID);
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            ESP_LOGD(TAG, "WIFI_STA_CONNECTED");
            delay(100);
#ifdef START_NETWORK_MD5_LCD
            DemoConsole.print("STA_CONNECTED\n");
#endif
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            //char ipv6[40];
            //snprintf(ipv6, sizeof(ipv6), IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
            ESP_LOGD(TAG, "WIFI STA_GOT_IP6: " IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
#ifdef START_NETWORK_MD5_LCD
            DemoConsole.print("STA IPv6: ");
            DemoConsole.writeMessage(IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
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

void StartNetworkClass::begin(const char* ssid, const char* password)
{
  // TODO: Should copy these strings, but we know they are referencing constants
  wifi_ssid = ssid;
  wifi_password = password;
  WiFi.disconnect(true);
  WiFi.onEvent(wifiOnEvent);
  //WiFi.mode(WIFI_MODE_APSTA);
  //WiFi.softAP(AP_SSID);
  //WiFi.begin(STA_SSID, STA_PASS);
  WiFi.enableIPv6();
  WiFi.begin(wifi_ssid, wifi_password);
};

const char * StartNetworkClass::eui64() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  // 01:34:67:9A:CD:F0
  // 0<3>3467 fffe 9acdf0
  snprintf(eui64_buffer, sizeof(eui64_buffer), "%02x%02x%02xfffe%02x%02x%02x", mac[0] ^ 2, mac[1], mac[2], mac[3], mac[4], mac[5]);
  return eui64_buffer;
}

IPAddress StartNetworkClass::globalIPv6(){
  IPAddress ip = WiFi.globalIPv6();
  if(ip.type() == IPType::IPv6 && ip != IN6ADDR_ANY){
    // Have global IPv6 address, so consider the network connected
    wifi_connected = true;
  }
  return ip;
}

String StartNetworkClass::mainDnsIP(){
  IPAddress dns = WiFi.dnsIP();
  return dns.toString();
}

bool StartNetworkClass::wifiConnected()
{
  return wifi_connected;
};

StartNetworkClass StartNetwork;
