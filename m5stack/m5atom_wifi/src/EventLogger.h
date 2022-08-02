#ifndef EventLogger_h
#define EventLogger_h

#include <esp_log.h>

class EventLogger
{
public:
  EventLogger();
  virtual void begin();
  virtual void error(const char *format, ...) __attribute__((format(printf, 2, 3)));
  virtual void information(const char *format, ...) __attribute__((format(printf, 2, 3)));
  virtual void loop();
  virtual void pending();
  virtual void success();
  virtual void warning();
protected:
  virtual void log(esp_log_level_t level, const char *message);
};

#endif
