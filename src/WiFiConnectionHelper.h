#ifndef WiFiConnectionHelper_h
#define WiFiConnectionHelper_h

#include <WiFi.h>
#include "Display.h"
#include "SysConfig.h"
#include "NuvIoTState.h"
#include "LedManager.h"
#include "NuvIoTState.h"
#include "Console.h"
#include "Hal.h"

typedef enum WiFi_Connection_State
{
    NuvIoTWiFi_NotConnected = 0,    
    NuvIoTWiFi_Connecting = 1,
    NuvIoTWiFi_Connected = 2,    
    NuvIoTWiFi_NotConfigured = 3,
} WiFi_Connection_State_t;

class WiFiConnectionHelper {
    public:
        WiFiConnectionHelper();
        WiFiConnectionHelper(WiFiClient *client, Display *display, LedManager *ledManager, NuvIoTState *state, Hal *hal, 
                             Console *console, SysConfig *sysConfig);

        void setup();
        void connect(bool isReconnecting);
        void disconnect();
        void loop();

        void post(String addr, uint16_t port, String path, String body);

        bool isConnected();
        String getIPAddress();
        String getMACAddress();
        String getWiFiStatus();
        uint8_t getRSSI();
          
    private:
        Display *m_display;
        WiFiClient *m_client;
        NuvIoTState *m_state;
        SysConfig *m_sysConfig;
        Console *m_console;
        LedManager *m_ledManager;

        long m_lastReconnect = 0;
        Hal *m_hal;
        int m_attempt = 0;
        int m_spinnerIndex = 0;
        bool m_isReconnect;
        WiFi_Connection_State_t m_wifiState = NuvIoTWiFi_NotConnected;
};

#endif