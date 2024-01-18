/*
#ifndef AtomLogger_h
#define AtomLogger_h

#include "EventLogger.h"

class AtomLogger : public EventLogger
{
public:
  AtomLogger();
  virtual void loop();
  virtual void pending();
  virtual void success();
  virtual void warning();
protected:
  virtual void log(esp_log_level_t level, const char *message);
};

#endif
*/
