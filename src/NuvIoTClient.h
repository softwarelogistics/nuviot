#ifndef NUVIOTCLIENT_H
#define NUVIOTCLIENT_H

#include "Display.h"
#include "Console.h"
#include "Hal.h"
#include "SIMModem.h"
#include "NuvIoTState.h"
#include "MQTT_GPRS.h"
#include "OtaServices.h"

class NuvIoTClient {
    private:
        Display *m_display;
        Hal *m_hal;
        SIMModem *m_modem;
        Console *m_console;
        MQTT *m_mqtt;
        NuvIoTState* m_state;
        String m_lastMsg;
        OtaServices *m_ota;

        bool m_gpsEnabled;

    public:
        NuvIoTClient(SIMModem *modem, MQTT *mqtt, Console *console, Display *display, NuvIoTState *state, OtaServices *ota, Hal *hal);
        bool ConnectToAPN(bool transparentMode, bool connectToAPN, unsigned long baudRate);
        bool Connect(bool isReconnect, unsigned long baudRate);
        void messagePublished(String topic, unsigned char *payload, size_t length);

        void enableGPS(bool enabled);

    private:
        void sendStatusUpdate(String msg, String nextAction);
        void sendStatusUpdate(String msg, String nextAction, String title, int delay);
};

#endif
