#ifndef NbIotNetworkService_h
#define NbIotNetworkService_h

#include "NetworkService.h"

#define TINY_GSM_MODEM_SIM7020

//#define SerialMon        Serial
//#define MONITOR_BAUDRATE 115200

#define SerialAT         Serial1
#define SIM7020_BAUDRATE 115200

#define ATOM_DTU_SIM7020_RESET -1
#define ATOM_DTU_SIM7020_EN    12
#define ATOM_DTU_SIM7020_TX    22
#define ATOM_DTU_SIM7020_RX    19

#define ATOM_DTU_RS485_TX 23
#define ATOM_DTU_RS485_RX 33

class NbIotNetworkService : public NetworkService
{
public:
  NbIotNetworkService();
  virtual void begin();
  virtual Client *createClient();
  virtual Client *createSecureClient(const char *rootCA);
  virtual bool isConnected();
  virtual void loop();
  void setApn(const char *apn);
  virtual void setEventLogger(EventLogger *eventLogger);
};

#endif
