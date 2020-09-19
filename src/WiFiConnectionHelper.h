#ifndef WiFiConnectionHelper_h
#define WiFiConnectionHelper_h

#include <WiFi.h>
#include "Display.h"
#include "SysConfig.h"
#include "NuvIoTState.h"
#include "NuvIoTState.h"

class WiFiConnectionHelper {
    public:
        WiFiConnectionHelper();
        WiFiConnectionHelper(WiFiClient *client, Display *display, NuvIoTState *state, SysConfig *sysConfig);
        void setup();
        void disconnect();
        void loop();

        bool isConnected();
        String getIPAddress();
        String getMACAddress();
          
    private:
        void connect(bool isReconnecting);
        Display *m_display;
        WiFiClient *m_client;
        NuvIoTState *m_state;
        SysConfig *m_sysConfig;
};

#endif