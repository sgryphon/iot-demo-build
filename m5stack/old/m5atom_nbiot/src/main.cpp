#include "AtomLogger.h"
#include "NbIotNetworkService.h"

#include <M5Atom.h>
#include <ArduinoHttpClient.h>

#define ST(A) #A
#define STR(A) ST(A)

int16_t _count = 0;
EventLogger *_logger = new AtomLogger();
const char *_nbiot_apn = STR(PIO_NBIOT_APN);
NbIotNetworkService *_network = nullptr;
extern const uint8_t _root_ca_pem_start[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_start");
extern const uint8_t _root_ca_pem_end[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_end");
const char *_version = STR(PIO_VERSION);

void testNetwork() {
  _logger->information("Button %d", _count);
  //logger->information("Button %d, IPv6 %s, IPv4 %s", count, network->globalIPv6().toString().c_str(), WiFi.localIP().toString().c_str());

  Client *client = _network->createClient();
  //WiFiClientSecure *client = new WiFiClientSecure;
  if (client == nullptr) {
    _logger->error("Unable to create secure client");
    return;
  }
  //client->setCACert((char *)root_ca_pem_start);

  const char *server = "v4v6.ipv6-test.com";
  const int port = 80;
  HttpClient *http = new HttpClient(*client, server, port);
  //bool success = http.begin(*client, "https://v4v6.ipv6-test.com/api/myip.php");
  if (http == nullptr) {
    _logger->error("Unable to create HTTP");
    delete client;
    return;
  }

  int rc = http->get("/api/myip.php");
  if (rc != 0) {
    _logger->error("HTTP GET error %d", rc);
    delete client;
    return;
  }

  int httpCode = http->responseStatusCode();
  if (httpCode != 200 && httpCode != 301) {
    _logger->error("HTTP GET error %d", httpCode);
    delete client;
    return;
  }

  String payload = http->responseBody();
  _logger->information("v4v6.ipv6-test.com=<%s>", payload.c_str());
  if (payload.indexOf(":") >= 0) {
    _logger->success();
  } else {
    _logger->warning();
  }
  delete client;
}

void setup() {
  M5.begin(true, true, true);
  delay(10);
  _logger->begin();
  _logger->information("Atom started, v%s", _version);

  NbIotNetworkService *nb_iot_network = new NbIotNetworkService();
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
