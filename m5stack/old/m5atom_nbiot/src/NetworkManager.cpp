#include "NetworkManager.h"

NetworkManager::NetworkManager() {}

void NetworkManager::begin() {}

Client *NetworkManager::createClient() { return nullptr; }

Client *NetworkManager::createSecureClient(const char *rootCA) { return nullptr; }

bool NetworkManager::isConnected() { return false; }

void NetworkManager::loop() {}

void NetworkManager::setEventLogger(EventLogger *eventLogger) {}
