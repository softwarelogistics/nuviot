#ifndef RELAYMANAGER_H
#define RELAYMANAGER_H

#include <Arduino.h>
#include <LagoVistaPins.h>

#define RELAY1_PIN 34
#define RELAY2_PIN 35
#define RELAY3_PIN 25
#define RELAY4_PIN 26

class RelayManager {
    public:
        RelayManager() {
            pinMode(K1_CTL, OUTPUT);
            pinMode(K2_CTL, OUTPUT);
            pinMode(K3_CTL, OUTPUT);
            pinMode(K4_CTL, OUTPUT);
        };

        void toggleRelay(int relayIndex, bool on){
            pinMode(K1_CTL, OUTPUT);
            pinMode(K2_CTL, OUTPUT);
            pinMode(K3_CTL, OUTPUT);
            pinMode(K4_CTL, OUTPUT);

            Serial.println("Relay " + String(relayIndex) + "  " + String(on));

            if(relayIndex == 0) digitalWrite(K1_CTL, on);
            if(relayIndex == 1) digitalWrite(K2_CTL, on);
            if(relayIndex == 2) digitalWrite(K3_CTL, on);
            if(relayIndex == 3) digitalWrite(K4_CTL, on);
        };
};

#endif