#ifndef RELAYMANAGER_H
#define RELAYMANAGER_H

#include <Arduino.h>

#define RELAY1_PIN 34
#define RELAY2_PIN 35
#define RELAY3_PIN 25
#define RELAY4_PIN 26

class RelayManager {
    public:
        RelayManager() {
            pinMode(RELAY1_PIN, OUTPUT);
            pinMode(RELAY2_PIN, OUTPUT);
            pinMode(RELAY3_PIN, OUTPUT);
            pinMode(RELAY4_PIN, OUTPUT);
        };

        void toggleRelay(int relayIndex, bool on){
            pinMode(RELAY1_PIN, OUTPUT);
            pinMode(RELAY2_PIN, OUTPUT);
            pinMode(RELAY3_PIN, OUTPUT);
            pinMode(RELAY4_PIN, OUTPUT);

            Serial.println("Relay " + String(relayIndex) + "  " + String(on));

            if(relayIndex == 0) digitalWrite(RELAY1_PIN, on);
            if(relayIndex == 1) digitalWrite(RELAY2_PIN, on);
            if(relayIndex == 2) digitalWrite(RELAY3_PIN, on);
            if(relayIndex == 3) digitalWrite(RELAY4_PIN, on);
        };
};

#endif