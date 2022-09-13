#include "EventLogger.h"

#include <Arduino.h>

static const char *TAG = "EventLogger";

EventLogger::EventLogger() {}

void EventLogger::begin() {
  ESP_LOGD(TAG, "Event logger begin");
}

void EventLogger::error(const char *format, ...) {
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

  log(ESP_LOG_ERROR, output);

  if (output != local_buffer) {
    free(output);
  }
}

void EventLogger::information(const char *format, ...) {
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

  log(ESP_LOG_INFO, output);

  if (output != local_buffer) {
    free(output);
  }
}

void EventLogger::loop() { }

void EventLogger::pending() { 
    ESP_LOGI(TAG, "Pending");
}

void EventLogger::success() { 
    ESP_LOGI(TAG, "Success");
}

void EventLogger::warning() { 
    ESP_LOGI(TAG, "Warning");
}

// Protected

void EventLogger::log(esp_log_level_t level, const char *message) {
  if (level == ESP_LOG_ERROR) {
    ESP_LOGE(TAG, "%s", message);
  } else {
    ESP_LOGI(TAG, "%s", message);
  }
}

