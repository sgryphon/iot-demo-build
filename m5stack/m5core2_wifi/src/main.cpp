#include <Arduino.h>
#include <M5Core2.h>
#include <WiFi.h>
//#include <WiFiMulti.h>
#include <HTTPClient.h>

#include "StartNetwork.h"
#include "wifi_config.h"

static const char* ssid = WIFI_SSID;
static const char* password = WIFI_PASSWORD;

#define SEND_INTERVAL_MS (10000)
unsigned long nextMessageMilliseconds = 0;
HTTPClient http;

#define HEADER_HEIGHT (16)

void printHeader() {
  // Time = 8, IPv6 = 39
  // Date = 10, MAC = 17, WiFi 3, IPv4 = 15
  RTC_DateTypeDef rtcDateNow;
  RTC_TimeTypeDef rtcTimeNow;
  M5.Rtc.GetDate(&rtcDateNow);
  M5.Rtc.GetTime(&rtcTimeNow);
  int x = M5.Lcd.getCursorX();
  int y = M5.Lcd.getCursorY();
  M5.Lcd.fillRect(0, 0, 320, HEADER_HEIGHT, M5.Lcd.color565(0x83, 0x4c, 0xc2));
  M5.Lcd.setTextColor(WHITE, M5.Lcd.color565(0x83, 0x4c, 0xc2));
  // Time
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("%02d:%02d:%02d", rtcTimeNow.Hours, rtcTimeNow.Minutes, rtcTimeNow.Seconds);
  // Date
  M5.Lcd.setCursor(0, 8);
  M5.Lcd.printf("%04d-%02d-%02d", rtcDateNow.Year, rtcDateNow.Month, rtcDateNow.Date);
  // IPv6
  String ipv6 = StartNetwork.globalIPv6().toString();
  M5.Lcd.setCursor(53 * 6 - M5.Lcd.textWidth(ipv6), 0); 
  //M5.Lcd.setCursor((53-39)*6, 0);
  M5.Lcd.print(ipv6);
  // MAC & WiFi Status
  M5.Lcd.setCursor((53-39)*6, 8);
  M5.Lcd.printf("%s (%3d)", WiFi.macAddress().c_str(), WiFi.status());
  // IPv4
  String ipv4 = WiFi.localIP().toString();
  M5.Lcd.setCursor(53 * 6 - M5.Lcd.textWidth(ipv4), 8); 
  M5.Lcd.print(ipv4.c_str());

  M5.Lcd.setCursor(x, y);
  M5.Lcd.setTextColor(WHITE, BLACK);
}

void printWiFi() {
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
}

void wifiConnectedLoop() {
  unsigned long nowMilliseconds = millis();
  if (nowMilliseconds > nextMessageMilliseconds) {
    nextMessageMilliseconds = nowMilliseconds + SEND_INTERVAL_MS;

    M5.Lcd.println(nowMilliseconds);
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
}


void setup() {
  M5.begin();

  printHeader();
  M5.Lcd.setCursor(0, HEADER_HEIGHT);
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
}

void loop() {
  M5.update();
  printHeader();
  if (M5.Lcd.getCursorY() > M5.Lcd.height()) {
    M5.Lcd.fillRect(0, HEADER_HEIGHT, 320, 240 - HEADER_HEIGHT, 0);
    M5.Lcd.setCursor(0, HEADER_HEIGHT);
    printWiFi();
  }

  if(StartNetwork.wifiConnected()){
    wifiConnectedLoop();
  }

  delay(1000);
}
