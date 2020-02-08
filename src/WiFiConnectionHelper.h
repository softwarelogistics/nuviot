#ifndef WiFiConnectionHelper_h
#define WiFiConnectionHelper_h

#include <WiFi.h>
#include "Display.h"
#include "NuvIoTState.h"

class WiFiConnectionHelper {
    public:
        WiFiConnectionHelper();
        WiFiConnectionHelper(WiFiClient *client, Display *display, NuvIoTState *state);
        void setup();
        void disconnect();
        void loop();
          
    private:
        void connect(bool isReconnecting);
        NuvIoTState *m_State;
        Display *m_display;
        WiFiClient *m_client;
};

#endif