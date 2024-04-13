#include "NetworkService.h"

NetworkService::NetworkService() {}

void NetworkService::begin() {}

Client *NetworkService::createClient() { return nullptr; }

Client *NetworkService::createSecureClient(const char *rootCA) { return nullptr; }

bool NetworkService::isConnected() { return false; }

void NetworkService::loop() {}

void NetworkService::setEventLogger(EventLogger *eventLogger) {}
