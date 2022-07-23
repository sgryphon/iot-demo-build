#include <Arduino.h>
#include <M5Core2.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#include "wifi_config.local.h"

static const char* ssid = IOT_CONFIG_WIFI_SSID;
static const char* password = IOT_CONFIG_WIFI_PASSWORD;

WiFiMulti wifiMulti;
HTTPClient http;

void setup() {
  M5.begin();
  wifiMulti.addAP(ssid, password);
  M5.Lcd.print("Connecting Wifi...\n");
}

void loop() {
  M5.Lcd.setCursor(0, 0);
  if ((wifiMulti.run() == WL_CONNECTED)) {  // wait for WiFi connection.
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
  } else {
    M5.Lcd.print("connect failed");
  }
  delay(5000);
  M5.Lcd.clear();
}
