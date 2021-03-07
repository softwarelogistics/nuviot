#ifndef NUVIOTCLIENT_H
#define NUVIOTCLIENT_H

#include "Display.h"
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
        Display *m_display;
        Hal *m_hal;
        SIMModem *m_modem;
        Console *m_console;
        MQTT *m_mqtt;
        NuvIoTState* m_state;
        String m_lastMsg;
        OtaServices *m_ota;
        SysConfig *m_sysConfig;
        LedManager *m_ledManager;
        String m_lastWarning;
        String m_lastError;

        bool m_gpsEnabled;

        void (*messageReceivedCallback)(String topic, unsigned char buffer[], size_t length) = NULL;

        void handleError(String errorCode, String message);
        void handleWarning(String errorCode, String message, int retryCount);
        void delayAndCheckState(long ms);

    public:
        NuvIoTClient(SIMModem *modem, MQTT *mqtt, Console *console, Display *display, LedManager *ledManager, NuvIoTState *state, SysConfig *sysConfig, OtaServices *ota, Hal *hal);
        NuvIoTClient(WiFiConnectionHelper *wifiConnectionHelper, NuvIoTMQTT *mqtt, Console *console, Display *display, LedManager *ledManager, NuvIoTState *state, SysConfig *sysConfig, OtaServices *ota, Hal *hal);
        bool ConnectToAPN(bool transparentMode, bool connectToAPN, unsigned long baudRate);
        bool CellularConnect(bool isReconnect, unsigned long baudRate);
        bool WifiConnect(bool isReconnect);
        void messagePublished(String topic, unsigned char *payload, size_t length);

        void setMessageReceivedCallback(void (*callback)(String topic, byte buffer[], size_t buffer_length)) {
            messageReceivedCallback = callback;
        }

        void enableGPS(bool enabled);

        void commonDisplay();

    private:
        void sendStatusUpdate(String msg, String nextAction);
        void sendStatusUpdate(String msg, String nextAction, String title, int delay);
};

#endif
