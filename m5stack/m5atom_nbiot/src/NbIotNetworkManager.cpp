#include "NbIotNetworkManager.h"

#include <Arduino.h>
#include <esp_log.h>

typedef enum {
  NOT_CONNECTED = 0,
  CONNECTING = 1,
  CONNECTED = 2
} NetworkStatus;

static const char *TAG = "Network";

char _apn[20] = { 0 };
EventLogger *_network_logger = nullptr;
uint16_t _retry_count = 0;
unsigned long _retry_at_millis = 0;
NetworkStatus _status = NOT_CONNECTED;

NbIotNetworkManager::NbIotNetworkManager() {
}

void NbIotNetworkManager::begin() {
  _network_logger->information("Begin NB-IoT Network Manager, APN: <%s>", _apn);
  _network_logger->pending();
}

bool NbIotNetworkManager::isConnected() { return _status == CONNECTED; }

void NbIotNetworkManager::loop() {
  if (_status == NOT_CONNECTED) {
    unsigned long now = millis();
    if (now > _retry_at_millis) {
      _status = CONNECTING;
    }
  }
}

void NbIotNetworkManager::setApn(const char *apn) {
  strcpy(_apn, apn);
}

void NbIotNetworkManager::setEventLogger(EventLogger *eventLogger) {
    _network_logger = eventLogger;
}
