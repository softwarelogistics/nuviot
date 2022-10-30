#ifndef NuvIoTMQTT_H
#define NuvIoTMQTT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "WiFiConnectionHelper.h"
#include "NuvIoTState.h"
#include "OtaServices.h"
#include "Hal.h"
#include "Display.h"
#include "Console.h"
#include "SysConfig.h"
#include "IOValues.h"
#include "RelayManager.h"

#define MAX_SUBSCRIPTION_COUNT 10

void handleMqttCallback(char *topic, byte *payload, unsigned int length);

class NuvIoTMQTT {
    private:
        void (*m_callback)(String topic, byte *buffer, size_t len) = NULL;

        int m_spinnerIndex = 0;
        int m_connectAttempt = 0;

        SysConfig *m_sysConfig = NULL;
        Console *m_console = NULL;
        NuvIoTState *m_state = NULL;
        PubSubClient *m_mqtt = NULL;
        Display *m_display = NULL;
        OtaServices *m_ota = NULL;
        Hal *m_hal = NULL;
        WiFiConnectionHelper *m_wifi;
        String m_subscriptions[MAX_SUBSCRIPTION_COUNT];
        bool m_transparentMode = true;
        int m_subscriptionCount;
        long m_lastPing = 0;
        long m_lastConnectAttempt = 0;

        String resolveConnectFail();        

    public:
        NuvIoTMQTT(WiFiConnectionHelper *wifiHelper, Console *console,  WiFiClient *client, Display *u8g2, OtaServices *ota, Hal *hal, NuvIoTState *state, SysConfig *sysConfig);
        
        void setup();
        void connect();
        void disconnect();
        void loop();
        bool isConnected();
        void addSubscriptions(String subscription);
        void publish(String topic, String payload);        
        void setMessageReceivedCallback(void (*callback)(String topic, unsigned char *buffer, size_t len));
        void handleMqttCallback(char *topic, byte *payload, unsigned int length);    
        void sendIOValues(IOValues *values);
        void sendRelayStatus(RelayManager *mgr);
};
#endif