#include "Hal.h"
#include <esp_task_wdt.h>

void Hal::restart(long pauseMS) {
    delay(pauseMS);
    ESP.restart();
}

void Hal::restart() {
    ESP.restart();
}

void Hal::enableHWWatchdog(long delaySeconds) {
    esp_task_wdt_init(delaySeconds, true); //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL); //add current thread to WDT watch
}

void Hal::disableHWWatchdog() {

}

void Hal::feedHWWatchdog() {
    esp_task_wdt_reset();
}