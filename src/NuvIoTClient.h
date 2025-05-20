#ifndef NUVIOTCLIENT_H
#define NUVIOTCLIENT_H

#ifdef LCD_DISPLAY
#include "Display.h"
#endif

#include "Console.h"
#include "Hal.h"
#include "SIMModem.h"
#include "NuvIoTState.h"
#include "MQTT_GPRS.h"
#include "OtaServices.h"
#include "SysConfig.h"
#include "LedManager.h"
#include "NuvIoTMQTT.h"

class NuvIoTClient {
    private:
        NuvIoTMQTT *m_nuviotMqtt = NULL;
        WiFiConnectionHelper *m_wifiConnectionHelper;
#ifdef LCD_DISPLAY
        Display *m_display;
#endif
        Hal *m_hal;

#ifdef CELLULAR_ENABLED
        SIMModem *m_modem;
        MQTT *m_cellMqtt = NULL;      
#endif

        Console *m_console;
        NuvIoTState* m_state;
        String m_lastMsg;
        OtaServices *m_ota;
        SysConfig *m_sysConfig;
        LedManager *m_ledManager;
        String m_lastWarning;
        String m_lastError;

        bool m_gpsEnabled;

        bool m_cellularConnection = false;
        bool m_wifiConnection = false;

        void (*m_commandHandler)(String topic, byte *buffer, size_t len) = NULL;

        void (*m_messageReceivedCallback)(String topic, unsigned char buffer[], size_t length) = NULL;

        void handleError(String errorCode, String message);
        void handleWarning(String errorCode, String message, int retryCount);
        void delayAndCheckState(long ms);

    public:
    #ifdef LCD_DISPLAY
        #ifdef CELLULAR_ENABLED
        NuvIoTClient(SIMModem *modem, WiFiConnectionHelper *wifiConnectionHelper, MQTT *cellMqtt, NuvIoTMQTT *wifiMqtt, Console *console, Display *display, LedManager *ledManager, NuvIoTState *state, SysConfig *sysConfig, OtaServices *ota, Hal *hal);
        #else
        NuvIoTClient(WiFiConnectionHelper *wifiConnectionHelper, NuvIoTMQTT *wifiMqtt, Console *console, Display *display, LedManager *ledManager, NuvIoTState *state, SysConfig *sysConfig, OtaServices *ota, Hal *hal);
        #endif
    #endif
 
    #ifdef CELLULAR_ENABLED
        NuvIoTClient(SIMModem *modem, WiFiConnectionHelper *wifiConnectionHelper, MQTT *cellMqtt, NuvIoTMQTT *wifiMqtt, Console *console, LedManager *ledManager, NuvIoTState *state, SysConfig *sysConfig, OtaServices *ota, Hal *hal);
    #else
        NuvIoTClient(WiFiConnectionHelper *wifiConnectionHelper, NuvIoTMQTT *wifiMqtt, Console *console, LedManager *ledManager, NuvIoTState *state, SysConfig *sysConfig, OtaServices *ota, Hal *hal);
    #endif

    #ifdef CELLULAR_ENABLED
        bool connectToAPN(bool transparentMode, bool connectToAPN, unsigned long baudRate);
        bool disconnectFromAPN();
        bool CellularConnect(bool isReconnect, unsigned long baudRate);
    #endif
    
    bool WifiConnect(bool isReconnect);
        void messagePublished(String topic, unsigned char *payload, size_t length);

        void setMessageReceivedCallback(void (*callback)(String topic, byte buffer[], size_t buffer_length)) {
            m_messageReceivedCallback = callback;
        }

        void setCommandHandlerCallback(void (*callback)(String topic, byte *buffer, size_t len)){
            m_commandHandler = callback;
        }

#ifdef CELLULAR_ENABLED        
        void enableGPS(bool enabled);
#endif

        void commonDisplay();

    private:
        void sendStatusUpdate(String msg, String nextAction);
        void sendStatusUpdate(String msg, String nextAction, String title, int delay);
};

#endif
