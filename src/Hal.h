#ifndef Hal_c
#define Hal_c

#include <Arduino.h>

class Hal {
    public:
        void restart();
        int getWiFi_RSSI();
};

#endif