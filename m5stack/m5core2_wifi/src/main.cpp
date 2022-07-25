#include <Arduino.h>
#include <M5Core2.h>
#include <WiFi.h>
//#include <WiFiMulti.h>
#include <HTTPClient.h>

#include "wifi_config.local.h"

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

void wifiOnConnect(){
    M5.Lcd.print("STA Connected\n");
    M5.Lcd.print("STA IPv4: ");
    M5.Lcd.print(WiFi.localIP());
    M5.Lcd.print("\n");

    Serial.println("STA Connected");
    Serial.print("STA IPv4: ");
    Serial.println(WiFi.localIP());
    
    //ntpClient.begin(2390);
}

void wifiOnDisconnect(){
    M5.Lcd.print("STA Disconnected\n");
    //Serial.println("STA Disconnected");
    delay(1000);
    WiFi.begin(ssid, password);
    //WiFi.begin(STA_SSID, STA_PASS);
}

void wifiConnectedLoop(){
  M5.Lcd.print("WiFi Connected loop...\n");
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
            M5.Lcd.print("STA_CONNECTED, enable IPv6\n");
            WiFi.enableIpV6();
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            //Serial.print("STA IPv6: ");
            //Serial.println(WiFi.localIPv6());
            M5.Lcd.print("STA IPv6: ");
            //M5.Lcd.print(info.got_ip6.ip6_info.);
            M5.Lcd.print(WiFi.localIPv6());
            M5.Lcd.print("\n");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            //Serial.print("AP IPv6: ");
            //Serial.println(WiFi.softAPIPv6());
            M5.Lcd.print("AP IPv6: ");
            M5.Lcd.print(WiFi.softAPIPv6());
            M5.Lcd.print("\n");
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
  M5.Lcd.print("Connecting...\n");

  Serial.begin(115200);
  Serial.println("STA Connecting");

  WiFi.disconnect(true);
  WiFi.onEvent(WiFiEvent);
  //WiFi.mode(WIFI_MODE_APSTA);
  //WiFi.softAP(AP_SSID);
  //WiFi.begin(STA_SSID, STA_PASS);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    M5.Lcd.print(".");
    Serial.print(".");
  }

  M5.Lcd.print("WiFi Connected\n");
  M5.Lcd.print("IPv4: ");
  M5.Lcd.print(WiFi.localIP());
  M5.Lcd.print("\n");

  M5.Lcd.print("IPv6: ");
  M5.Lcd.print(WiFi.localIPv6());
  M5.Lcd.print("\n");

  //WiFi.enableIpV6();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

void loop() {
  M5.update();

  M5.Rtc.GetDate(&rtcDateNow);
  M5.Rtc.GetTime(&rtcTimeNow);
  
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);

  M5.Lcd.printf("Clock %04d-%02d-%02d %02d:%02d:%02d\n",
    rtcDateNow.Year, rtcDateNow.Month, rtcDateNow.Date,
    rtcTimeNow.Hours, rtcTimeNow.Minutes, rtcTimeNow.Seconds);

  M5.Lcd.printf("WiFi Status: %d\n", WiFi.status());
  M5.Lcd.print("IPv4: ");
  M5.Lcd.print(WiFi.localIP());
  M5.Lcd.print("\n");
  M5.Lcd.print("IPv6: ");
  M5.Lcd.print(WiFi.localIPv6());
  M5.Lcd.print("\n");

  if(wifi_connected){
      //wifiConnectedLoop();
  }
  delay(10000);
}
