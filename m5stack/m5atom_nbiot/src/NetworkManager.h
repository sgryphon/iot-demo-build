#ifndef NetworkManager_h
#define NetworkManager_h

#include "EventLogger.h"

#include <Client.h>

class NetworkManager
{
public:
  NetworkManager();
  virtual Client *createClient();
  virtual Client *createSecureClient(const char *rootCA);
  virtual void begin();
  virtual bool isConnected();
  virtual void loop();
  virtual void setEventLogger(EventLogger *eventLogger);
};

#endif
