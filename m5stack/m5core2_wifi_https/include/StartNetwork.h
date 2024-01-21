#ifndef StartNetwork_h
#define StartNetwork_h

#include <WiFi.h>
#include <IPAddress.h>

class StartNetworkClass
{

public:
    void begin(const char* ssid, const char* password);
    const char * eui64();
    bool wifiConnected();
};

extern StartNetworkClass StartNetwork;

#endif
