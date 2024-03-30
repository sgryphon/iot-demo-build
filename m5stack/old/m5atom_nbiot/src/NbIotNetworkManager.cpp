#include "NbIotNetworkService.h"

#include <TinyGsmClient.h>
#include <Arduino.h>
#include <esp_log.h>

typedef enum {
  NOT_CONNECTED = 0,
  CONNECTING = 1,
  CONNECTED = 2
} NetworkStatus;

static const char *TAG = "NbIot";

char _apn[20] = { 0 };
EventLogger *_network_logger = nullptr;
uint16_t _retry_count = 0;
unsigned long _retry_at_millis = 0;
NetworkStatus _status = NOT_CONNECTED;

TinyGsm modem(SerialAT, ATOM_DTU_SIM7020_RESET);
//TinyGsmClient tcpClient(modem);

void nbConnect(void) {
  unsigned long start = millis();
  ESP_LOGD(TAG, "Initializing modem");
  while (!modem.init()) {
    ESP_LOGD(TAG, "Initializing modem %ds", (millis() - start) / 1000);
  };

  start = millis();
  ESP_LOGD(TAG, "Waiting for network");
  while (!modem.waitForNetwork()) {
    ESP_LOGD(TAG, "Waiting for network %ds", (millis() - start) / 1000);
  }
  _network_logger->information("Success");
  _network_logger->success();
}

NbIotNetworkService::NbIotNetworkService() {
}

void NbIotNetworkService::begin() {
  _network_logger->information("Begin NB-IoT Network Manager, APN: <%s>", _apn);
  _network_logger->pending();

  SerialAT.begin(SIM7020_BAUDRATE, SERIAL_8N1, ATOM_DTU_SIM7020_RX,
                  ATOM_DTU_SIM7020_TX);
  nbConnect();
}

Client *NbIotNetworkService::createClient() {
  TinyGsmClient *tcp_client = new TinyGsmClient(modem);
  return tcp_client;
  //return tcpClient;
}

Client *NbIotNetworkService::createSecureClient(const char *rootCA) {
  return nullptr;
}

bool NbIotNetworkService::isConnected() { return _status == CONNECTED; }

void NbIotNetworkService::loop() {
  if (_status == NOT_CONNECTED) {
    unsigned long now = millis();
    if (now > _retry_at_millis) {
      _status = CONNECTING;
    }
  }
}

void NbIotNetworkService::setApn(const char *apn) {
  strcpy(_apn, apn);
}

void NbIotNetworkService::setEventLogger(EventLogger *eventLogger) {
    _network_logger = eventLogger;
}
