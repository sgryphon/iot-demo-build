#include "Core2Logger.h"

#include <M5Unified.h>
#include <WiFi.h>

#define ST(A) #A
#define STR(A) ST(A)

Core2Logger::Core2Logger() {}

static const char *TAG = "Core2Logger";
static const char* version = STR(PIO_VERSION);
static const int HEADER_HEIGHT = 16;

unsigned long header_update_at_millis = 0;
unsigned long header_update_interval_ms = 500;

/*
CRGB led_color = CRGB::White;
int16_t led_count_remaining = 0;
int16_t led_interval_ms = 500;
unsigned long led_on_at_millis = 0;
unsigned long led_off_at_millis = 0;
int16_t led_time_on_ms = 200;
*/

/*
void ledOn(unsigned long now) {
  led_off_at_millis = now + led_time_on_ms;
  // Next on
  if (led_count_remaining == 0) {
    led_on_at_millis = 0;
  } else {
    led_on_at_millis = now + led_interval_ms;
    if (led_count_remaining > 0) {
      led_count_remaining--;
    }
  }
  ESP_LOGD(TAG, "LED on %08x (off %d)", led_color, led_off_at_millis);
  M5.dis.fillpix(led_color);
}
*/

void printHeader() {
  // Time = 8, IPv6 = 39
  // Date = 10, WiFi 3, IPv4 = 15, Version/MAC = 17
  uint16_t headerColor;
  IPAddress globalAddress = WiFi.globalIPv6();
  if (WiFi.isConnected()) {
    if (globalAddress.type() == IPType::IPv6 && globalAddress != IN6ADDR_ANY) {
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
  String ipv6 = globalAddress.toString();
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

/*
void startLed(CRGB color, int16_t count, int16_t time_on_ms = 200) {
  led_color = color;
  led_time_on_ms = time_on_ms;
  led_count_remaining = count > 0 ? count - 1 : count;
  ledOn(millis());
}
*/

// Public

void Core2Logger::loop() {
  unsigned long now = millis();
  /*
  if (led_off_at_millis > 0 && now > led_off_at_millis) {
    led_off_at_millis = 0;
    ESP_LOGD(TAG, "LED off");
    M5.dis.clear();
  }
  if (led_on_at_millis > 0 && now > led_on_at_millis) {
    ledOn(now);
  }
  */
  if (now > header_update_at_millis) {
    printHeader();
    if (header_update_at_millis == 0) {
      header_update_at_millis = now + header_update_interval_ms;
    } else {
      header_update_at_millis += header_update_interval_ms;
    }
  }
}

void Core2Logger::pending() { 
  //startLed(CRGB::Yellow, -1);
  ESP_LOGI(TAG, "Pending");
}

void Core2Logger::success() { 
  //startLed(CRGB::Green, 2, 500);
  ESP_LOGI(TAG, "Success");
}

void Core2Logger::warning() { 
  //startLed(CRGB::Orange, 1, 400);
  ESP_LOGW(TAG, "Warning");
}

// Protected

void Core2Logger::log(esp_log_level_t level, const char *message) {
  if (level == ESP_LOG_ERROR) {
    //startLed(CRGB::Red, 3);
    ESP_LOGE(TAG, "CORE2: %s", message);
  } else {
    //startLed(CRGB::Blue, 1);
    ESP_LOGI(TAG, "CORE2: %s", message);
  }
}
