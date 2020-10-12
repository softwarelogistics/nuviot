#include "Hal.h"

void Hal::restart(long pauseMS) {
    delay(pauseMS);
    ESP.restart();
}

void Hal::restart() {
    ESP.restart();
}