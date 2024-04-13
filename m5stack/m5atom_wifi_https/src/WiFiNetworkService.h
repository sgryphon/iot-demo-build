#ifndef WiFiNetworkService_h
#define WiFiNetworkService_h

#include "NetworkService.h"

#include <WiFi.h>

class WiFiNetworkService : public NetworkService
{
public:
  WiFiNetworkService();
  virtual void begin();
  const char * eui64();
  IPv6Address globalIPv6();
  virtual bool isConnected();
  virtual void loop();
  String mainDnsIP();
  void setCredentials(const char *ap_password, const char *ssid, const char *password);
  virtual void setEventLogger(EventLogger *eventLogger);
};

#endif