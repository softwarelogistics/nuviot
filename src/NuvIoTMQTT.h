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

#define MAX_SUBSCRIPTION_COUNT 10

void handleMqttCallback(char *topic, byte *payload, unsigned int length);

class NuvIoTMQTT {
    private:
        void (*m_callback)(String topic, unsigned char *buffer, size_t len) = NULL;

        SysConfig *m_sysConfig = NULL;
        Console *m_console = NULL;
        NuvIoTState *m_state = NULL;
        PubSubClient *m_mqtt = NULL;
        Display *m_display = NULL;
        OtaServices *m_ota = NULL;
        Hal *m_hal = NULL;
        WiFiClient *m_client;
        WiFiConnectionHelper *m_wifi;
        String m_subscriptions[MAX_SUBSCRIPTION_COUNT];
        bool m_transparentMode = true;
        int m_subscrptionCount;


        String resolveConnectFail();        

    public:
        NuvIoTMQTT(WiFiConnectionHelper *wifiHelper, Console *console,  WiFiClient *client, Display *u8g2, OtaServices *ota, Hal *hal, NuvIoTState *state, SysConfig *sysConfig);
        
        void setup();
        void connect();
        void loop();
        void addSubscriptions(String subscription);
        void publish(String topic, String payload);        
        void registerCallback(void (*callback)(String topic, unsigned char *buffer, size_t len));
        void handleMqttCallback(char *topic, byte *payload, unsigned int length);
};
#endif