#ifndef RELAYMANAGER_H
#define RELAYMANAGER_H

#define MAX_RELAYS 8

#include <Arduino.h>
#include "Console.h"
#include "ConfigPins.h"
#include "IOConfig.h"
#include "AbstractSensor.h"

class RelayManager : public AbstractSensor {
    private: 
        bool m_state[MAX_RELAYS];
        String m_names[MAX_RELAYS];
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

            if (m_configPins->K1Ctl != -1 && m_ioConfig->Relay1Enabled) { pinMode(m_configPins->K1Ctl, OUTPUT); digitalWrite(m_configPins->K1Ctl, LOW);}
            if (m_configPins->K2Ctl != -1 && m_ioConfig->Relay2Enabled) { pinMode(m_configPins->K2Ctl, OUTPUT); digitalWrite(m_configPins->K2Ctl, LOW);}
            if (m_configPins->K3Ctl != -1 && m_ioConfig->Relay3Enabled) { pinMode(m_configPins->K3Ctl, OUTPUT); digitalWrite(m_configPins->K3Ctl, LOW);}
            if (m_configPins->K4Ctl != -1 && m_ioConfig->Relay4Enabled) { pinMode(m_configPins->K4Ctl, OUTPUT); digitalWrite(m_configPins->K4Ctl, LOW);} 
            if (m_configPins->K5Ctl != -1 && m_ioConfig->Relay5Enabled) { pinMode(m_configPins->K5Ctl, OUTPUT); digitalWrite(m_configPins->K5Ctl, LOW);}
            if (m_configPins->K6Ctl != -1 && m_ioConfig->Relay6Enabled) { pinMode(m_configPins->K6Ctl, OUTPUT); digitalWrite(m_configPins->K6Ctl, LOW);}
            if (m_configPins->K7Ctl != -1 && m_ioConfig->Relay7Enabled) { pinMode(m_configPins->K7Ctl, OUTPUT); digitalWrite(m_configPins->K7Ctl, LOW);}
            if (m_configPins->K8Ctl != -1 && m_ioConfig->Relay8Enabled) { pinMode(m_configPins->K8Ctl, OUTPUT); digitalWrite(m_configPins->K8Ctl, LOW);}
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

            if(relayIndex == 0 && m_configPins->K1Ctl != -1 && m_ioConfig->Relay1Enabled) digitalWrite(m_configPins->K1Ctl, on);
            if(relayIndex == 1 && m_configPins->K2Ctl != -1 && m_ioConfig->Relay2Enabled) digitalWrite(m_configPins->K2Ctl, on);
            if(relayIndex == 2 && m_configPins->K3Ctl != -1 && m_ioConfig->Relay3Enabled) digitalWrite(m_configPins->K3Ctl, on);
            if(relayIndex == 3 && m_configPins->K4Ctl != -1 && m_ioConfig->Relay4Enabled) digitalWrite(m_configPins->K4Ctl, on);
            if(relayIndex == 4 && m_configPins->K5Ctl != -1 && m_ioConfig->Relay5Enabled) digitalWrite(m_configPins->K5Ctl, on);
            if(relayIndex == 5 && m_configPins->K6Ctl != -1 && m_ioConfig->Relay6Enabled) digitalWrite(m_configPins->K6Ctl, on);
            if(relayIndex == 6 && m_configPins->K7Ctl != -1 && m_ioConfig->Relay7Enabled) digitalWrite(m_configPins->K7Ctl, on);
            if(relayIndex == 7 && m_configPins->K8Ctl != -1 && m_ioConfig->Relay8Enabled) digitalWrite(m_configPins->K8Ctl, on);
        };

        void loop() {            
            
        }

        void debugPrint() {
            if(m_configPins->K1Ctl != -1 && m_ioConfig->Relay1Enabled) m_console->printVerbose(m_ioConfig->Relay1Name + "=" + String(m_state[0] ? "on" : "off") + "; Pin: " + String(m_configPins->K1Ctl));
            if(m_configPins->K2Ctl != -1 && m_ioConfig->Relay2Enabled) m_console->printVerbose(m_ioConfig->Relay2Name + "=" + String(m_state[1] ? "on" : "off") + "; Pin: " + String(m_configPins->K2Ctl));
            if(m_configPins->K3Ctl != -1 && m_ioConfig->Relay3Enabled) m_console->printVerbose(m_ioConfig->Relay3Name + "=" + String(m_state[2] ? "on" : "off") + "; Pin: " + String(m_configPins->K3Ctl));
            if(m_configPins->K4Ctl != -1 && m_ioConfig->Relay4Enabled) m_console->printVerbose(m_ioConfig->Relay4Name + "=" + String(m_state[3] ? "on" : "off") + "; Pin: " + String(m_configPins->K4Ctl));
            if(m_configPins->K5Ctl != -1 && m_ioConfig->Relay5Enabled) m_console->printVerbose(m_ioConfig->Relay5Name + "=" + String(m_state[4] ? "on" : "off") + "; Pin: " + String(m_configPins->K5Ctl));
            if(m_configPins->K6Ctl != -1 && m_ioConfig->Relay6Enabled) m_console->printVerbose(m_ioConfig->Relay6Name + "=" + String(m_state[5] ? "on" : "off") + "; Pin: " + String(m_configPins->K6Ctl));
            if(m_configPins->K7Ctl != -1 && m_ioConfig->Relay7Enabled) m_console->printVerbose(m_ioConfig->Relay7Name + "=" + String(m_state[6] ? "on" : "off") + "; Pin: " + String(m_configPins->K7Ctl));
            if(m_configPins->K8Ctl != -1 && m_ioConfig->Relay8Enabled) m_console->printVerbose(m_ioConfig->Relay8Name + "=" + String(m_state[7] ? "on" : "off") + "; Pin: " + String(m_configPins->K8Ctl));
        }

        String toString() {
            String state = m_state[0] ? "1" : "0";
            for(int idx = 1; idx < MAX_RELAYS; ++idx) {
                state += (m_state[idx] ? ",1" : ",0");
            }

            return state;
        }
};

#endif