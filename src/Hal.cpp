#include "Hal.h"
#include <esp_task_wdt.h>

void Hal::restart(long pauseMS)
{
    delay(pauseMS);
    ESP.restart();
}

void Hal::restart()
{
    ESP.restart();
}

void Hal::enableHWWatchdog(long delaySeconds)
{
    esp_task_wdt_init(delaySeconds, true); //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL);                //add current thread to WDT watch
}

void Hal::disableHWWatchdog()
{
}

void Hal::feedHWWatchdog()
{
    esp_task_wdt_reset();
}

reset_reason_t Hal::resetReason()
{
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason)
    {
    case ESP_RST_UNKNOWN:
        return RST_UNKNOWN; //!< Reset reason can not be determined
    case ESP_RST_POWERON:
        return RST_POWERON; //!< Reset due to power-on event
    case ESP_RST_EXT:
        return RST_EXT; //!< Reset by external pin (not applicable for ESP32)
    case ESP_RST_SW:
        return RST_SW; //!< Software reset via esp_restart
    case ESP_RST_PANIC:
        return RST_PANIC; //!< Software reset due to exception/panic
    case ESP_RST_INT_WDT:
        return RST_INT_WDT; //!< Reset (software or hardware) due to interrupt watchdog
    case ESP_RST_TASK_WDT:
        return RST_TASK_WDT; //!< Reset due to task watchdog
    case ESP_RST_WDT:
        return RST_WDT; //!< Reset due to other watchdogs
    case ESP_RST_DEEPSLEEP:
        return RST_DEEPSLEEP; //!< Reset after exiting deep sleep mode
    case ESP_RST_BROWNOUT:
        return RST_BROWNOUT; //!< Brownout reset (software or hardware)
    case ESP_RST_SDIO:
        return RST_SDIO; //!< Reset over SDIO
    }
    return (reset_reason_t)esp_reset_reason();
}

String Hal::resetReasonMessage()
{
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason)
    {
    case ESP_RST_UNKNOWN:
        return "Uknown"; //!< Reset reason can not be determined
    case ESP_RST_POWERON:
        return "Power On"; //!< Reset due to power-on event
    case ESP_RST_EXT:
        return "External Pin"; //!< Reset by external pin (not applicable for ESP32)
    case ESP_RST_SW:
        return "Software Reset"; //!< Software reset via esp_restart
    case ESP_RST_PANIC:
        return "Panic"; //!< Software reset due to exception/panic
    case ESP_RST_INT_WDT:
        return "Interrupt Watchdog"; //!< Reset (software or hardware) due to interrupt watchdog
    case ESP_RST_TASK_WDT:
        return "Hardare Watchdog"; //!< Reset due to task watchdog
    case ESP_RST_WDT:
        return "Other Watchdog"; //!< Reset due to other watchdogs
    case ESP_RST_DEEPSLEEP:
        return "Deep Sleep"; //!< Reset after exiting deep sleep mode
    case ESP_RST_BROWNOUT:
        return "Brownout"; //!< Brownout reset (software or hardware)
    case ESP_RST_SDIO:
        return "Reset over SDIO"; //!< Reset over SDIO
    }
    return "?";
}