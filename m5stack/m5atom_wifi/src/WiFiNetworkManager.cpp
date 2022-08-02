#include "WiFiNetworkManager.h"

#include <Arduino.h>

bool connected_ = false;
const char *ssid_ = { 0 };
const char *password_ = { 0 };

WiFiNetworkManager::WiFiNetworkManager() {
}

void WiFiNetworkManager::begin() {
  eventLogger()->information("Begin WiFi Network Manager, SSID: %s", ssid_);
  eventLogger()->pending();
}

bool WiFiNetworkManager::isConnected() { return connected_; }

void WiFiNetworkManager::loop() {
  if (!connected_) {
    if (millis() > 5000) {
      connected_ = true;
      eventLogger()->information("Connected");
      eventLogger()->ready();
    }
  }
}

void WiFiNetworkManager::setCredentials(const char *ssid, const char *password) {
    ssid_ = ssid;
    password_ = password;
}
