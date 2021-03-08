#ifndef WiFiConnectionHelper_h
#define WiFiConnectionHelper_h

#include <WiFi.h>
#include "Display.h"
#include "SysConfig.h"
#include "NuvIoTState.h"
#include "NuvIoTState.h"
#include "Console.h"
#include "Hal.h"

class WiFiConnectionHelper {
    public:
        WiFiConnectionHelper();
        WiFiConnectionHelper(WiFiClient *client, Display *display, NuvIoTState *state, Hal *hal, Console *console, SysConfig *sysConfig);
        void setup();
        void connect(bool isReconnecting);
        void disconnect();
        void loop();

        bool isConnected();
        String getIPAddress();
        String getMACAddress();
        String getWiFiStatus(int);
        int getRSSI();
          
    private:
        Display *m_display;
        WiFiClient *m_client;
        NuvIoTState *m_state;
        SysConfig *m_sysConfig;
        Console *m_console;
        Hal *m_hal;
};

#endif