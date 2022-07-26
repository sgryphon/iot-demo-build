#include <Arduino.h>
#include <M5Core2.h>
#include <WiFi.h>
//#include <WiFiMulti.h>
#include <HTTPClient.h>

#include "StartNetwork.h"
#include "wifi_config.h"

static const char* ssid = WIFI_SSID;
static const char* password = WIFI_PASSWORD;

//WiFiMulti wifiMulti;
HTTPClient http;
//WiFiUDP ntpClient;
RTC_DateTypeDef rtcDateNow;
RTC_TimeTypeDef rtcTimeNow;

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
  M5.Lcd.print(StartNetwork.globalIPv6());
  M5.Lcd.print("\n");
  M5.Lcd.print("DNS: ");
  M5.Lcd.print(StartNetwork.mainDnsIP());
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


void wifiConnectedLoop(){
  //M5.Lcd.print("WiFi Connected loop...\n");
  printWiFi();

  M5.Lcd.print("v6: begin");
  http.begin("http://v6.ipv6-test.com/api/myip.php");
  M5.Lcd.print(",GET");
  int httpCode = http.GET();
  M5.Lcd.printf(",%d", httpCode);
  if (httpCode > 0) {  // httpCode will be negative on error.
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      M5.Lcd.print(",");
      M5.Lcd.print(payload);
    }
  } else {
    M5.Lcd.printf(",ERROR %s", http.errorToString(httpCode).c_str());
  }
  M5.Lcd.print("\n");
  http.end();

  M5.Lcd.print("v4v6: begin");
  http.begin("http://v4v6.ipv6-test.com/api/myip.php");
  M5.Lcd.print(",GET");
  httpCode = http.GET();
  M5.Lcd.printf(",%d", httpCode);
  if (httpCode > 0) {  // httpCode will be negative on error.
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      M5.Lcd.print(",");
      M5.Lcd.print(payload);
    }
  } else {
    M5.Lcd.printf(",ERROR %s", http.errorToString(httpCode).c_str());
  }
  M5.Lcd.print("\n");
  http.end();
}


void setup() {
  M5.begin();

  printWiFi();

  M5.Lcd.printf("Connecting to %s", ssid);
  M5.Lcd.print("\n");

  if (ssid=="") {
    M5.Lcd.print("SSID missing");
    M5.Lcd.print("\n");
    return;
  }
  if (password=="") {
    M5.Lcd.print("Password missing");
    M5.Lcd.print("\n");
    return;
  }

  Serial.begin(115200);
  Serial.println("STA Connecting");

  StartNetwork.begin(ssid, password);
  delay(1000);

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

  if(StartNetwork.wifiConnected()){
    wifiConnectedLoop();
  }

  delay(10000);
}
