#ifndef WiFiNetworkManager_h
#define WiFiNetworkManager_h

#include "NetworkManager.h"

#include <WiFi.h>

class WiFiNetworkManager : public NetworkManager
{
public:
  WiFiNetworkManager();
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
