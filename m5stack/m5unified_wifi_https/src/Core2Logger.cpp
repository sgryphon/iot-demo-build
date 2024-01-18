#include "Core2Logger.h"

#include <M5Unified.h>
#include <WiFi.h>

#define ST(A) #A
#define STR(A) ST(A)

typedef enum {
  NONE = 0,
  PENDING = 1,
  SUCCESS = 2,
  WARNING = 3,
  ERROR = 4
} LoggerStatus;

static const char *TAG = "Core2Logger";
static const char* version = STR(PIO_VERSION);
static const int HEADER_HEIGHT = 16;

std::mutex mutex;
unsigned long header_update_at_millis = 0;
unsigned long header_update_interval_ms = 500;
LoggerStatus status = NONE;

Core2Logger::Core2Logger() {}

void checkPage() {
  if (M5.Lcd.getCursorY() > M5.Lcd.height()) {
    M5.Lcd.fillRect(0, HEADER_HEIGHT, 320, 240 - HEADER_HEIGHT, 0);
    M5.Lcd.setCursor(0, HEADER_HEIGHT);
  }
}

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
  IPAddress ipv6Address = WiFi.globalIPv6();
  bool hasGlobalIpv6 = ipv6Address != IN6ADDR_ANY;
  if (!hasGlobalIpv6) {
    ipv6Address = WiFi.localIPv6();
  }

  uint16_t headerColor;
  if(status == PENDING) {
    headerColor = ORANGE;
  } else if (status == SUCCESS) {
    if (hasGlobalIpv6) {
      headerColor = DARKGREEN;
    } else {
      headerColor = BLUE;
    }
  } else if(status == WARNING) {
    headerColor = ORANGE;
  } else if(status == ERROR) {
    headerColor = RED;
  } else {
    headerColor = DARKGREY;
  }

  m5::rtc_datetime_t now = M5.Rtc.getDateTime();
  String ipv6 = ipv6Address.toString();
  String ipv4 = WiFi.localIP().toString();
  wl_status_t wifi_status = WiFi.status();

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
  M5.Lcd.printf("(%2d)", wifi_status);
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

void Core2Logger::begin() {
  std::lock_guard<std::mutex> lck(mutex); 
  ESP_LOGD(TAG, "Core2 logger begin");
  M5.Lcd.setCursor(0, HEADER_HEIGHT);
  printHeader();
}

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
    {
      std::lock_guard<std::mutex> lck(mutex); 
      printHeader();
    }
    if (header_update_at_millis == 0) {
      header_update_at_millis = now + header_update_interval_ms;
    } else {
      header_update_at_millis += header_update_interval_ms;
    }
  }
}

void Core2Logger::pending() { 
  //startLed(CRGB::Yellow, -1);
  status = PENDING;
  ESP_LOGI(TAG, "Pending");
}

void Core2Logger::success() { 
  //startLed(CRGB::Green, 2, 500);
  status = SUCCESS;
  ESP_LOGI(TAG, "Success");
}

void Core2Logger::warning() {
  //startLed(CRGB::Orange, 1, 400);
  status = WARNING;
  ESP_LOGW(TAG, "Warning");
}

// Protected

void Core2Logger::log(esp_log_level_t level, const char *message) {
  std::lock_guard<std::mutex> lck(mutex);
  if (level == ESP_LOG_ERROR) {
    //startLed(CRGB::Red, 3);
    status = ERROR;
    ESP_LOGE(TAG, "CORE2: %s", message);
    checkPage();
    M5.Lcd.printf("ERROR: %s", message);
    M5.Lcd.println();
  } else {
    //startLed(CRGB::Blue, 1);
    ESP_LOGI(TAG, "CORE2: %s", message);
    checkPage();
    M5.Lcd.println(message);
  }
}
