#ifndef RELAYMANAGER_H
#define RELAYMANAGER_H

#include <Arduino.h>
#include <Console.h>
#include <LagoVistaPins.h>

#define RELAY1_PIN 27
#define RELAY2_PIN 14
#define RELAY3_PIN 12
#define RELAY4_PIN 13

class RelayManager {
    private: 
        bool m_state[4];
        Console *m_console;
    public:
        RelayManager(Console *console) {
            m_console = console;
            setup();
        };

        void setup() { 
            pinMode(RELAY1_PIN, OUTPUT);
            pinMode(RELAY2_PIN, OUTPUT);
            pinMode(RELAY3_PIN, OUTPUT);
            pinMode(RELAY4_PIN, OUTPUT);
        }

        void setRelay(int relayIndex, bool on){
            m_state[relayIndex] = on;            

            if(relayIndex == 0) digitalWrite(K1_CTL, on);
            if(relayIndex == 1) digitalWrite(K2_CTL, on);
            if(relayIndex == 2) digitalWrite(K3_CTL, on);
            if(relayIndex == 3) digitalWrite(K4_CTL, on);
        };

        void debugPrint() {
            m_console->printVerbose("relay1=" + String(m_state[0] ? "on" : "off"));
            m_console->printVerbose("relay2=" + String(m_state[1] ? "on" : "off"));
            m_console->printVerbose("relay3=" + String(m_state[2] ? "on" : "off"));
            m_console->printVerbose("relay4=" + String(m_state[3] ? "on" : "off"));
        }
};

#endif