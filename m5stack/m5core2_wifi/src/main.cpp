#include <Arduino.h>
#include <M5Core2.h>
#include <WiFi.h>
//#include <WiFiMulti.h>
#include <HTTPClient.h>

#include "wifi_config.h"

//#define STA_SSID "**********"
//#define STA_PASS "**********"
#define AP_SSID  "esp32-v6"

static volatile bool wifi_connected = false;

static const char* ssid = WIFI_SSID;
static const char* password = WIFI_PASSWORD;

//WiFiMulti wifiMulti;
HTTPClient http;
//WiFiUDP ntpClient;
RTC_DateTypeDef rtcDateNow;
RTC_TimeTypeDef rtcTimeNow;

esp_netif_t* get_esp_interface_netif(esp_interface_t interface);
IPv6Address globalIPv6(){
	esp_ip6_addr_t addr;
  if(WiFiGenericClass::getMode() == WIFI_MODE_NULL){
    return IPv6Address();
  }
  if(esp_netif_get_ip6_global(get_esp_interface_netif(ESP_IF_WIFI_STA), &addr)) {
    return IPv6Address();
  }
  return IPv6Address(addr.addr);
}

String mainDnsIP(){
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

void printWiFi() {
  M5.Rtc.GetDate(&rtcDateNow);
  M5.Rtc.GetTime(&rtcTimeNow);

  M5.Lcd.setCursor(0, 0);

  M5.Lcd.printf("Clock %04d-%02d-%02d %02d:%02d:%02d",
    rtcDateNow.Year, rtcDateNow.Month, rtcDateNow.Date,
    rtcTimeNow.Hours, rtcTimeNow.Minutes, rtcTimeNow.Seconds);
  M5.Lcd.print("\n");

  M5.Lcd.printf("WiFi Status: %3d", WiFi.status());
  M5.Lcd.print("\n");
  M5.Lcd.print("\n");

  M5.Lcd.print("IPv6: ");
  M5.Lcd.print(WiFi.localIPv6());
  M5.Lcd.print("\n");
  M5.Lcd.print("IPv6: ");
  M5.Lcd.print(globalIPv6());
  M5.Lcd.print("\n");
  M5.Lcd.print("DNS: ");
  M5.Lcd.print(mainDnsIP());
  M5.Lcd.print("\n");
  M5.Lcd.print("\n");

  M5.Lcd.print("IPv4: ");
  M5.Lcd.print(WiFi.localIP());
  M5.Lcd.print("\n");
  M5.Lcd.print("DNS: ");
  M5.Lcd.print(WiFi.dnsIP(0));
  M5.Lcd.print("\n");
  M5.Lcd.print("Gateway: ");
  M5.Lcd.print(WiFi.gatewayIP());
  M5.Lcd.print("\n");
  M5.Lcd.print("\n");

  int16_t x = M5.Lcd.getCursorX();
  int16_t y = M5.Lcd.getCursorY();
  M5.Lcd.fillRect(x, y, 320 - x, 240 - y, 0);
}

void wifiOnConnect(){
  M5.Lcd.print("STA Connected");
  M5.Lcd.print("          \n");
  M5.Lcd.print("STA IPv4: ");
  M5.Lcd.print(WiFi.localIP());
  M5.Lcd.print("          \n");

  Serial.println("STA Connected");
  Serial.print("STA IPv4: ");
  Serial.println(WiFi.localIP());
    
    //ntpClient.begin(2390);
}

void wifiOnDisconnect(){
  M5.Lcd.print("STA Disconnected");
  M5.Lcd.print("          \n");
  //Serial.println("STA Disconnected");
  delay(1000);
  WiFi.begin(ssid, password);
  //WiFi.begin(STA_SSID, STA_PASS);
}

void wifiConnectedLoop(){
  //M5.Lcd.print("WiFi Connected loop...\n");
  printWiFi();

/*
  M5.Lcd.print("[HTTP] begin...\n");
  http.begin("https://zenquotes.io/api/random");
  M5.Lcd.print("[HTTP] GET...\n");
  int httpCode = http.GET();
  if (httpCode > 0) {  // httpCode will be negative on error.
    M5.Lcd.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      M5.Lcd.println(payload);
    }
  } else {
    M5.Lcd.printf("[HTTP] GET... failed, error: %s\n",
                  http.errorToString(httpCode).c_str());
  }
  http.end();
  */
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
            M5.Lcd.print("STA_CONNECTED, enable IPv6");
            M5.Lcd.print("          \n");
            WiFi.enableIpV6();
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            //Serial.print("STA IPv6: ");
            //Serial.println(WiFi.localIPv6());
            M5.Lcd.print("STA IPv6: ");
            M5.Lcd.printf(IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
            //M5.Lcd.print(WiFi.localIPv6());
            M5.Lcd.print("          \n");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            //Serial.print("AP IPv6: ");
            //Serial.println(WiFi.softAPIPv6());
            M5.Lcd.print("AP IPv6: ");
            M5.Lcd.print(WiFi.softAPIPv6());
            M5.Lcd.print("          \n");
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

void setup() {
  M5.begin();

  printWiFi();

  M5.Lcd.printf("Connecting to %s", ssid);
  M5.Lcd.print("          \n");

  if (ssid=="") {
    M5.Lcd.print("SSID missing");
    M5.Lcd.print("          \n");
    return;
  }
  if (password=="") {
    M5.Lcd.print("Password missing");
    M5.Lcd.print("          \n");
    return;
  }

  Serial.begin(115200);
  Serial.println("STA Connecting");

  WiFi.disconnect(true);
  WiFi.onEvent(WiFiEvent);
  //WiFi.mode(WIFI_MODE_APSTA);
  //WiFi.softAP(AP_SSID);
  //WiFi.begin(STA_SSID, STA_PASS);
  WiFi.begin(ssid, password);

/*
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    M5.Lcd.print(".");
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  */
}

void loop() {
  M5.update();

  if(wifi_connected){
    wifiConnectedLoop();
  }

  delay(5000);
}
