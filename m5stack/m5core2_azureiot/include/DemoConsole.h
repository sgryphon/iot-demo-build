#ifndef DemoConsole_h
#define DemoConsole_h

class DemoConsoleClass {

public:
  void begin();
  void loop();
  void writeMessage(const char *format, ...) __attribute__((format(printf, 2, 3)));
  //    size_t printf_P(PGM_P format, ...) __attribute__((format(printf, 2,
  //    3)));
  //size_t print(const __FlashStringHelper *);
  //size_t print(const String &);
  //size_t print(const Printable &);
};

extern DemoConsoleClass DemoConsole;

#endif