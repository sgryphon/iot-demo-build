#include "AtomLogger.h"
#include "NbIotNetworkManager.h"

#include <HTTPClient.h>
#include <M5Atom.h>

#define ST(A) #A
#define STR(A) ST(A)

int16_t _count = 0;
EventLogger *_logger = new AtomLogger();
const char *_nbiot_apn = STR(PIO_NBIOT_APN);
NbIotNetworkManager *_network = nullptr;
extern const uint8_t _root_ca_pem_start[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_start");
extern const uint8_t _root_ca_pem_end[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_end");
const char *_version = STR(PIO_VERSION);

void testNetwork() {
  _logger->information("Button %d", _count);
  //logger->information("Button %d, IPv6 %s, IPv4 %s", count, network->globalIPv6().toString().c_str(), WiFi.localIP().toString().c_str());

/*
  WiFiClientSecure *client = new WiFiClientSecure;
  if (!client) {
    logger->error("Unable to create secure client");
    return;
  }
  client->setCACert((char *)root_ca_pem_start);
*/

/*
  HTTPClient http;
  bool success = http.begin(*client, "https://v4v6.ipv6-test.com/api/myip.php");
  if (!success) {
    logger->error("Unable to begin HTTP");
    delete client;
    return;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_MOVED_PERMANENTLY) {
    logger->error("HTTP GET error %d: %s", httpCode, http.errorToString(httpCode).c_str());
    delete client;
    return;
  }

  String payload = http.getString();
  logger->information("v4v6.ipv6-test.com=<%s>", payload.c_str());
  if (payload.indexOf(":") >= 0) {
    logger->success();
  } else {
    logger->warning();
  }
  delete client;
  */
}

void setup() {
  M5.begin(true, true, true);
  delay(10);
  _logger->begin();
  _logger->information("Atom started, v%s", _version);

  NbIotNetworkManager *nb_iot_network = new NbIotNetworkManager();
  nb_iot_network->setEventLogger(_logger);
  nb_iot_network->setApn(_nbiot_apn);
  _network = nb_iot_network;
  _network->begin();
}

void loop() {  
  M5.update();
  _logger->loop();
  _network->loop();
  if (M5.Btn.wasPressed()) {
    ++_count;
    if (_count % 5 == 0) {
      _logger->error("Too many button presses");
      return;
    }
    testNetwork();
    //logger->information("Button was pressed %d\n", count);
  }
}
