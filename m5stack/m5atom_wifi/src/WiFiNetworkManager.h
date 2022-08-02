#ifndef WiFiNetworkManager_h
#define WiFiNetworkManager_h

#include "NetworkManager.h"

class WiFiNetworkManager : public NetworkManager
{
public:
  WiFiNetworkManager();
  virtual void begin();
  virtual bool isConnected();
  virtual void loop();
  void setCredentials(const char *ssid, const char *password);
};

#endif
