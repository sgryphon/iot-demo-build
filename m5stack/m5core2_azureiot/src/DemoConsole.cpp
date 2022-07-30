#include "DemoConsole.h"
#include "StartNetwork.h"

#include <M5Core2.h>
#include <esp_log.h>

static const char *TAG = "demo";

#define HEADER_HEIGHT (16)

void checkPage() {
  if (M5.Lcd.getCursorY() > M5.Lcd.height()) {
    M5.Lcd.fillRect(0, HEADER_HEIGHT, 320, 240 - HEADER_HEIGHT, 0);
    M5.Lcd.setCursor(0, HEADER_HEIGHT);
  }
}

void printHeader() {
  // Time = 8, IPv6 = 39
  // Date = 10, MAC = 17, WiFi 3, IPv4 = 15
  uint16_t header_color;
  String ipv6 = StartNetwork.globalIPv6().toString();
  if (StartNetwork.wifiConnected()) {
    if (ipv6 != "0000:0000:0000:0000:0000:0000:0000:0000") {
      header_color = DARKGREEN;
    } else {
      header_color = BLUE;
    }
  } else {
    header_color = ORANGE;
  }
  RTC_TimeTypeDef rtc_time_now;
  RTC_DateTypeDef rtc_date_now;
  M5.Rtc.GetTime(&rtc_time_now);
  M5.Rtc.GetDate(&rtc_date_now);
  int x = M5.Lcd.getCursorX();
  int y = M5.Lcd.getCursorY();
  M5.Lcd.fillRect(0, 0, 320, HEADER_HEIGHT, header_color);
  M5.Lcd.setTextColor(WHITE, header_color);
  // Time
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("%02d:%02d:%02d", rtc_time_now.Hours, rtc_time_now.Minutes,
                rtc_time_now.Seconds);
  // Date
  M5.Lcd.setCursor(0, 8);
  M5.Lcd.printf("%04d-%02d-%02d", rtc_date_now.Year, rtc_date_now.Month,
                rtc_date_now.Date);
  // IPv6
  M5.Lcd.setCursor(53 * 6 - M5.Lcd.textWidth(ipv6), 0);
  // M5.Lcd.setCursor((53-39)*6, 0);
  M5.Lcd.print(ipv6);
  // MAC & WiFi Status
  M5.Lcd.setCursor((53 - 39) * 6, 8);
  M5.Lcd.printf("%s (%3d)", WiFi.macAddress().c_str(), WiFi.status());
  // IPv4
  String ipv4 = WiFi.localIP().toString();
  M5.Lcd.setCursor(53 * 6 - M5.Lcd.textWidth(ipv4), 8);
  M5.Lcd.print(ipv4.c_str());

  M5.Lcd.setCursor(x, y);
  M5.Lcd.setTextColor(WHITE, BLACK);
}

void DemoConsoleClass::begin() {
  ESP_LOGD(TAG, "DemoConsole::begin");
  printHeader();
  M5.Lcd.setCursor(0, HEADER_HEIGHT);
}

void DemoConsoleClass::loop() { printHeader(); }

void DemoConsoleClass::writeMessage(const char *format, ...) {
  char local_buffer[64];
  char *output = local_buffer;
  va_list arg;
  va_list copy;
  va_start(arg, format);
  va_copy(copy, arg);
  int16_t len = vsnprintf(output, sizeof(local_buffer), format, copy);
  va_end(copy);
  if (len < 0) {
    va_end(arg);
    return;
  };
  // if too big for local_buffer, then allocate space and do it all again
  if (len >= sizeof(local_buffer)) {
    output = (char *)malloc(len + 1);
    if (output == NULL) {
      va_end(arg);
      return;
    }
    len = vsnprintf(output, len + 1, format, arg);
  }
  va_end(arg);

  // len = write((uint8_t*)temp, len);
  checkPage();
  M5.Lcd.println(output);
  ESP_LOGI(TAG, "%s", output);

  if (output != local_buffer) {
    free(output);
  }
}

// size_t DemoConsoleClass::printf_P(PGM_P format, ...)
// __attribute__((format(printf, 2, 3))) {
//     M5.Lcd.printf()
// }

// size_t DemoConsoleClass::print(const __FlashStringHelper *ifsh) {
//   check_page();
//   M5.Lcd.print(ifsh);
//   return Serial.print(ifsh);
// }

// size_t DemoConsoleClass::print(const String &s) {
//   check_page();
//   M5.Lcd.print(s);
//   return Serial.print(s);
// }

// size_t DemoConsoleClass::print(const Printable &x) {
//   check_page();
//   M5.Lcd.print(x);
//   return Serial.print(x);
// }

DemoConsoleClass DemoConsole;
