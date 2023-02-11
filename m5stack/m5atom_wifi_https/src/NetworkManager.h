#ifndef NetworkManager_h
#define NetworkManager_h

#include "EventLogger.h"

class NetworkManager
{
public:
  NetworkManager();
  virtual void begin();
  virtual bool isConnected();
  virtual void loop();
  virtual void setEventLogger(EventLogger *eventLogger);
};

#endif
