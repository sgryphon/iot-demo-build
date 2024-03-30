#ifndef NetworkService_h
#define NetworkService_h

#include "EventLogger.h"

class NetworkService
{
public:
  NetworkService();
  virtual void begin();
  virtual bool isConnected();
  virtual void loop();
  virtual void setEventLogger(EventLogger *eventLogger);
};

#endif
