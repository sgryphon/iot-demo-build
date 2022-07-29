#ifndef StartNetwork_h
#define StartNetwork_h

#include <WiFi.h>

class StartNetworkClass {

public:
  void begin(const char *ssid, const char *password);
  const char *eui64();
  IPv6Address globalIPv6();
  String mainDnsIP();
  bool wifiConnected();
};

extern StartNetworkClass StartNetwork;

#endif