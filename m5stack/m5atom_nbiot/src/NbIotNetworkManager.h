#ifndef NbIotNetworkManager_h
#define NbIotNetworkManager_h

#include "NetworkManager.h"


class NbIotNetworkManager : public NetworkManager
{
public:
  NbIotNetworkManager();
  virtual void begin();
  virtual bool isConnected();
  virtual void loop();
  void setApn(const char *apn);
  virtual void setEventLogger(EventLogger *eventLogger);
};

#endif
