#include "NetworkManager.h"

EventLogger *eventLogger_ = nullptr;

NetworkManager::NetworkManager() {}

void NetworkManager::begin() {}

bool NetworkManager::isConnected() { return false; }

void NetworkManager::loop() {}

EventLogger *NetworkManager::eventLogger() { return eventLogger_; }

void NetworkManager::setEventLogger(EventLogger *eventLogger) {
    eventLogger_ = eventLogger;
}
