#include "NetworkManager.h"

NetworkManager::NetworkManager() {}

void NetworkManager::begin() {}

Client *NetworkManager::createClient() { return nullptr; }

Client *NetworkManager::createSecureClient(const char *root_ca) { return nullptr; }

bool NetworkManager::isConnected() { return false; }

void NetworkManager::loop() {}

void NetworkManager::setEventLogger(EventLogger *event_logger) {}
