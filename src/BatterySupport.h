#ifndef BATTERY_SUPPORT_H
#define BATTERY_SUPPORT_H


#include "Console.h"
#include "MessagePayload.h"
#include "ConfigPins.h"

class BatterySupport
{
private:
    Console *m_console;
    ConfigPins *m_configPins;
    MessagePayload *m_payload;  

public:
    BatterySupport(Console *console, ConfigPins *configPins, MessagePayload *payload) {
        m_console = console;
        m_configPins = configPins;
        m_payload = payload;
    }   

    void setup();
    void loop();

    bool hasBatterySupport();
    bool isUsbPresent();
    bool isBatteryCharging();
    float readBatteryVoltage();
};


#endif
