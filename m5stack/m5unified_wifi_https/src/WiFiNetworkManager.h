#ifndef WiFiNetworkManager_h
#define WiFiNetworkManager_h

#include "NetworkManager.h"

#include <WiFi.h>

class WiFiNetworkManager : public NetworkManager
{
public:
  WiFiNetworkManager();
  virtual void begin();
  virtual bool isConnected();
  virtual void loop();
  void setCredentials(const char *ap_password, const char *ssid, const char *password);
  virtual void setEventLogger(EventLogger *eventLogger);
};

#endif
