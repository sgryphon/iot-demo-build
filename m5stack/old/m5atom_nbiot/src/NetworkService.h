#ifndef NetworkService_h
#define NetworkService_h

#include "EventLogger.h"

#include <Client.h>

class NetworkService
{
public:
  NetworkService();
  virtual Client *createClient();
  virtual Client *createSecureClient(const char *rootCA);
  virtual void begin();
  virtual bool isConnected();
  virtual void loop();
  virtual void setEventLogger(EventLogger *eventLogger);
};

#endif
