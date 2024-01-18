#include <M5Unified.h>
#include "DemoConsole.h"
#include "StartNetwork.h"

#include "esp_log.h"
static const char* TAG = "demo";

#define HEADER_HEIGHT (16)

#define ST(A) #A
#define STR(A) ST(A)

static const char* version = STR(PIO_VERSION);

void checkPage() {
  if (M5.Lcd.getCursorY() > M5.Lcd.height()) {
    M5.Lcd.fillRect(0, HEADER_HEIGHT, 320, 240 - HEADER_HEIGHT, 0);
    M5.Lcd.setCursor(0, HEADER_HEIGHT);
  }
}

void printHeader() {
  // Time = 8, IPv6 = 39
  // Date = 10, WiFi 3, IPv4 = 15, Version/MAC = 17
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
  m5::rtc_datetime_t now = M5.Rtc.getDateTime();
  int x = M5.Lcd.getCursorX();
  int y = M5.Lcd.getCursorY();
  M5.Lcd.fillRect(0, 0, 320, HEADER_HEIGHT, headerColor);
  M5.Lcd.setTextColor(WHITE, headerColor);
  // Time
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("%02d:%02d:%02d", now.time.hours, now.time.minutes, now.time.seconds);
  // Date
  M5.Lcd.setCursor(0, 8);
  M5.Lcd.printf("%04d-%02d-%02d", now.date.year, now.date.month, now.date.date);
  // WiFi Status
  M5.Lcd.setCursor(9 * 6, 0);
  M5.Lcd.printf("(%2d)", WiFi.status());
  // Version 
  M5.Lcd.setCursor(11 * 6, 8); 
  M5.Lcd.printf("v%s", version);
  // IPv6
  M5.Lcd.setCursor(53 * 6 - M5.Lcd.textWidth(ipv6), 0); 
  M5.Lcd.print(ipv6);
  // (or MAC)
  // String mac = WiFi.macAddress();
  // M5.Lcd.setCursor(53 * 6 - M5.Lcd.textWidth(mac), 8); 
  // M5.Lcd.print(mac.c_str());
  //M5.Lcd.setCursor((53-39)*6, 0);
  // IPv4
  String ipv4 = WiFi.localIP().toString();
  M5.Lcd.setCursor(53*6 - M5.Lcd.textWidth(ipv4) - 6, 8);
  M5.Lcd.printf(" %s", ipv4.c_str());

  M5.Lcd.setCursor(x, y);
  M5.Lcd.setTextColor(WHITE, BLACK);
}

void DemoConsoleClass::begin() {
  ESP_LOGD(TAG, "DemoConsole::begin");
  printHeader();
  M5.Lcd.setCursor(0, HEADER_HEIGHT);
}

void DemoConsoleClass::loop() {
  printHeader();
}

size_t DemoConsoleClass::writeMessage(const char * format, ...) {
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
  checkPage();
  M5.Lcd.print(temp);
  Serial.print(temp);
  
  if(temp != loc_buf){
    free(temp);
  }
  return len;
}

//size_t DemoConsoleClass::printf_P(PGM_P format, ...) __attribute__((format(printf, 2, 3))) {
//    M5.Lcd.printf()
//}

size_t DemoConsoleClass::print(const __FlashStringHelper *ifsh) {
  checkPage();
  M5.Lcd.print(ifsh);
  return Serial.print(ifsh);
}

size_t DemoConsoleClass::print(const String &s) {
  checkPage();
  M5.Lcd.print(s);
  return Serial.print(s);
}

size_t DemoConsoleClass::print(const Printable& x) {
  checkPage();
  M5.Lcd.print(x);
  return Serial.print(x);
}

DemoConsoleClass DemoConsole;
