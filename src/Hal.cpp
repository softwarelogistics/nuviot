#include "Hal.h"
#include <WiFi.h>

void Hal::restart() {
    ESP.restart();
}

int Hal::getWiFi_RSSI(){
    int points = 10;

    long rssi = 0;
    long averageRSSI = 0;
    
    for (int i=0;i < points;i++){
        rssi += WiFi.RSSI();
        delay(20);
    }

   averageRSSI = rssi/points;
    return averageRSSI;
}