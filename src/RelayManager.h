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
        bool m_state[5];
        Console *m_console;
        ConfigPins *m_configPins;
        IOConfig *m_ioConfig;

    public:
        RelayManager(Console *console, ConfigPins *configPins) {
            m_console = console;
            m_configPins = configPins;    
        };

        void setup(IOConfig *ioConfig) {
            m_ioConfig = ioConfig; 

            if (m_configPins->K1Ctl != -1) pinMode(m_configPins->K1Ctl, OUTPUT);
            if (m_configPins->K2Ctl != -1) pinMode(m_configPins->K2Ctl, OUTPUT);
            if (m_configPins->K3Ctl != -1) pinMode(m_configPins->K3Ctl, OUTPUT);
            if (m_configPins->K4Ctl != -1) pinMode(m_configPins->K4Ctl, OUTPUT);
            if (m_configPins->K5Ctl != -1) pinMode(m_configPins->K5Ctl, OUTPUT);
        }

        void configure(IOConfig *ioConfig) {
            m_ioConfig = ioConfig; 
        }

        bool getRelayState(int relayIndex)
        {
            return m_state[relayIndex];
        }

        void setRelay(int relayIndex, bool on){
            m_state[relayIndex] = on;            

            if(relayIndex == 0 && m_configPins->K1Ctl != -1) digitalWrite(m_configPins->K1Ctl, on);
            if(relayIndex == 1 && m_configPins->K2Ctl != -1) digitalWrite(m_configPins->K2Ctl, on);
            if(relayIndex == 2 && m_configPins->K3Ctl != -1) digitalWrite(m_configPins->K3Ctl, on);
            if(relayIndex == 3 && m_configPins->K4Ctl != -1) digitalWrite(m_configPins->K4Ctl, on);
            if(relayIndex == 4 && m_configPins->K5Ctl != -1) digitalWrite(m_configPins->K5Ctl, on);
        };

        void loop() {            
            
        }

        void debugPrint() {
            if(m_configPins->K1Ctl != -1) m_console->printVerbose("relay1=" + String(m_state[0] ? "on" : "off") + "; Pin: " + String(m_configPins->K1Ctl));
            if(m_configPins->K2Ctl != -2) m_console->printVerbose("relay2=" + String(m_state[1] ? "on" : "off") + "; Pin: " + String(m_configPins->K2Ctl));
            if(m_configPins->K3Ctl != -3) m_console->printVerbose("relay3=" + String(m_state[2] ? "on" : "off") + "; Pin: " + String(m_configPins->K3Ctl));
            if(m_configPins->K4Ctl != -4) m_console->printVerbose("relay4=" + String(m_state[3] ? "on" : "off") + "; Pin: " + String(m_configPins->K4Ctl));
            if(m_configPins->K5Ctl != -5) m_console->printVerbose("relay5=" + String(m_state[4] ? "on" : "off") + "; Pin: " + String(m_configPins->K5Ctl));
        }
};

#endif