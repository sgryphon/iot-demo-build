#ifndef Core2Logger_h
#define Core2Logger_h

#include "EventLogger.h"

class Core2Logger : public EventLogger
{
public:
  Core2Logger();
  virtual void begin();
  virtual void loop();
  virtual void pending();
  virtual void success();
  virtual void warning();
protected:
  virtual void log(esp_log_level_t level, const char *message);
};

#endif
