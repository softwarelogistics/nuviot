#ifndef RELAYMANAGER_H
#define RELAYMANAGER_H

#include <Arduino.h>
#include "Console.h"
#include "ConfigPins.h"
#include "IOConfig.h"
#include "AbstractSensor.h"

#define RELAY1_PIN 27
#define RELAY2_PIN 14
#define RELAY3_PIN 12
#define RELAY4_PIN 13

class RelayManager : public AbstractSensor {
    private: 
        bool m_state[4];
        Console *m_console;
        ConfigPins *m_configPins;
        IOConfig *m_ioConfig;

    public:
        RelayManager(Console *console, ConfigPins *configPins) {
            m_console = console;
            m_configPins = configPins;
     
            pinMode(m_configPins->K1Ctl, OUTPUT);
            pinMode(m_configPins->K2Ctl, OUTPUT);
            pinMode(m_configPins->K3Ctl, OUTPUT);
            pinMode(m_configPins->K4Ctl, OUTPUT);
        };

        void setup(IOConfig *ioConfig) {
            m_ioConfig = ioConfig; 
        }

        void configure(IOConfig *ioConfig) {
            m_ioConfig = ioConfig; 
        }

        void setRelay(int relayIndex, bool on){
            m_state[relayIndex] = on;            

            if(relayIndex == 0) digitalWrite(m_configPins->K1Ctl, on);
            if(relayIndex == 1) digitalWrite(m_configPins->K2Ctl, on);
            if(relayIndex == 2) digitalWrite(m_configPins->K3Ctl, on);
            if(relayIndex == 3) digitalWrite(m_configPins->K4Ctl, on);
        };

        void loop() {
            
        }

        void debugPrint() {
            m_console->printVerbose("relay1=" + String(m_state[0] ? "on" : "off"));
            m_console->printVerbose("relay2=" + String(m_state[1] ? "on" : "off"));
            m_console->printVerbose("relay3=" + String(m_state[2] ? "on" : "off"));
            m_console->printVerbose("relay4=" + String(m_state[3] ? "on" : "off"));
        }
};

#endif