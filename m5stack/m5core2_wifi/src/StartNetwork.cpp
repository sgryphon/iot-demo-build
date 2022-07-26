#include "StartNetwork.h"

//#define START_NETWORK_MD5_LCD 1
//#ifdef START_NETWORK_MD5_LCD
#include <M5Core2.h>
//#endif

#include <WiFi.h>

static const char* _ssid = NULL;
static const char* _password = NULL;

//#define STA_SSID "**********"
//#define STA_PASS "**********"
#define AP_SSID  "esp32-v6"

static volatile bool wifi_connected = false;

esp_netif_t* get_esp_interface_netif(esp_interface_t interface);

void wifiOnConnect(){
#ifdef START_NETWORK_MD5_LCD
  M5.Lcd.print("STA Connected");
  M5.Lcd.print("          \n");
  M5.Lcd.print("STA IPv4: ");
  M5.Lcd.print(WiFi.localIP());
  M5.Lcd.print("          \n");
#endif
/*
  Serial.println("STA Connected");
  Serial.print("STA IPv4: ");
  Serial.println(WiFi.localIP());
    */
    //ntpClient.begin(2390);
}

void wifiOnDisconnect(){
#ifdef START_NETWORK_MD5_LCD
  M5.Lcd.print("STA Disconnected");
  M5.Lcd.print("          \n");
#endif
  //Serial.println("STA Disconnected");
  delay(1000);
  WiFi.begin(_ssid, _password);
  //WiFi.begin(STA_SSID, STA_PASS);
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
            delay(100);
            WiFi.enableIpV6();
//            M5.Lcd.print("STA_CONNECTED, enable IPv6");
#ifdef START_NETWORK_MD5_LCD
            M5.Lcd.print("STA_CONNECTED, ** enable IPv6 **");
            M5.Lcd.print("          \n");
#endif
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            //Serial.print("STA IPv6: ");
            //Serial.println(WiFi.localIPv6());
//            M5.Lcd.printf(IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
#ifdef START_NETWORK_MD5_LCD
            M5.Lcd.print("STA IPv6: ");
            M5.Lcd.printf(IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
            //M5.Lcd.print(WiFi.localIPv6());
            M5.Lcd.print("          \n");
#endif
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            //Serial.print("AP IPv6: ");
            //Serial.println(WiFi.softAPIPv6());
#ifdef START_NETWORK_MD5_LCD
            M5.Lcd.print("AP IPv6: ");
            M5.Lcd.print(WiFi.softAPIPv6());
            M5.Lcd.print("          \n");
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