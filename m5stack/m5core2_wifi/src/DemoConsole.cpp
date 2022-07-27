#include <M5Core2.h>
#include "DemoConsole.h"
#include "StartNetwork.h"

#include "esp_log.h"
static const char* TAG = "demo";

#define HEADER_HEIGHT (16)

void printHeader() {
  // Time = 8, IPv6 = 39
  // Date = 10, MAC = 17, WiFi 3, IPv4 = 15
  uint16_t headerColor;
  String ipv6 = StartNetwork.globalIPv6().toString();
  if (StartNetwork.wifiConnected()) {
    if (ipv6 != "0000:0000:0000:0000:0000:0000:0000:0000") {
      headerColor = DARKGREEN;
    } else {
      headerColor = BLUE;
    }
  } else {
    headerColor = ORANGE;
  }
  RTC_DateTypeDef rtcDateNow;
  RTC_TimeTypeDef rtcTimeNow;
  M5.Rtc.GetDate(&rtcDateNow);
  M5.Rtc.GetTime(&rtcTimeNow);
  int x = M5.Lcd.getCursorX();
  int y = M5.Lcd.getCursorY();
  M5.Lcd.fillRect(0, 0, 320, HEADER_HEIGHT, headerColor);
  M5.Lcd.setTextColor(WHITE, headerColor);
  // Time
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("%02d:%02d:%02d", rtcTimeNow.Hours, rtcTimeNow.Minutes, rtcTimeNow.Seconds);
  // Date
  M5.Lcd.setCursor(0, 8);
  M5.Lcd.printf("%04d-%02d-%02d", rtcDateNow.Year, rtcDateNow.Month, rtcDateNow.Date);
  // IPv6
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

void DemoConsoleClass::begin() {
  ESP_LOGD(TAG, "printWiFi");
  printHeader();
  M5.Lcd.setCursor(0, HEADER_HEIGHT);
}

void DemoConsoleClass::loop() {
  printHeader();

  if (M5.Lcd.getCursorY() > M5.Lcd.height()) {
    M5.Lcd.fillRect(0, HEADER_HEIGHT, 320, 240 - HEADER_HEIGHT, 0);
    M5.Lcd.setCursor(0, HEADER_HEIGHT);
  }
}

size_t DemoConsoleClass::printf(const char * format, ...) {
    char loc_buf[64];
    char * temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    if(len < 0) {
        va_end(arg);
        return 0;
    };
    if(len >= sizeof(loc_buf)){
        temp = (char*) malloc(len+1);
        if(temp == NULL) {
            va_end(arg);
            return 0;
        }
        len = vsnprintf(temp, len+1, format, arg);
    }
    va_end(arg);
    
    //len = write((uint8_t*)temp, len);
    M5.Lcd.print(temp);
    
    if(temp != loc_buf){
        free(temp);
    }
    return len;
}

//size_t DemoConsoleClass::printf_P(PGM_P format, ...) __attribute__((format(printf, 2, 3))) {
//    M5.Lcd.printf()
//}

size_t DemoConsoleClass::print(const __FlashStringHelper *ifsh) {
    return M5.Lcd.print(ifsh);
}

size_t DemoConsoleClass::print(const String &s) {
    return M5.Lcd.print(s);
}

size_t DemoConsoleClass::print(const Printable& x) {
    return M5.Lcd.print(x);
}

DemoConsoleClass DemoConsole;
