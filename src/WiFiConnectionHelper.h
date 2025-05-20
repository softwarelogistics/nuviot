#ifndef WiFiConnectionHelper_h
#define WiFiConnectionHelper_h

#include <WiFi.h>

#ifdef LCD_DISPLAY
#include "Display.h"
#endif

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
#ifdef LCD_DISPLAY        
        WiFiConnectionHelper(WiFiClient *client, Display *display, LedManager *ledManager, NuvIoTState *state, Hal *hal, 
                             Console *console, SysConfig *sysConfig);
#endif

        WiFiConnectionHelper(WiFiClient *client, LedManager *ledManager, NuvIoTState *state, Hal *hal, 
                             Console *console, SysConfig *sysConfig);

        String siteSurvey();
        void setup();
        void disconnect();
        void loop();

        void post(String addr, uint16_t port, String path, String body);
        bool connect(bool isReconnecting);
     
        bool isConnected();
        String getIPAddress();
        String getMACAddress();
        String getWiFiStatus();
        int32_t getRSSI();
          
    private:
#ifdef LCD_DISPLAY
        Display *m_display;
#endif
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