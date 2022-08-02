#include "WiFiNetworkManager.h"

const char *ssid_ = { 0 };
const char *password_ = { 0 };

WiFiNetworkManager::WiFiNetworkManager() {
}

void WiFiNetworkManager::begin() {
  eventLogger()->information("Begin WiFi Network Manager, SSID: %s", ssid_);
}

void WiFiNetworkManager::loop() {}

void WiFiNetworkManager::setCredentials(const char *ssid, const char *password) {
    ssid_ = ssid;
    password_ = password;
}
