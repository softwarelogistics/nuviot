#ifndef HAL_C
#define HAL_C

#include <Arduino.h>

typedef enum {
    RST_UNKNOWN = 100,    //!< Reset reason can not be determined
    RST_POWERON = 101,    //!< Reset due to power-on event
    RST_EXT = 102,        //!< Reset by external pin (not applicable for ESP32)
    RST_SW = 103,         //!< Software reset via esp_restart
    RST_PANIC = 104,      //!< Software reset due to exception/panic
    RST_INT_WDT = 105,    //!< Reset (software or hardware) due to interrupt watchdog
    RST_TASK_WDT = 106,   //!< Reset due to task watchdog
    RST_WDT = 107,        //!< Reset due to other watchdogs
    RST_DEEPSLEEP = 108,  //!< Reset after exiting deep sleep mode
    RST_BROWNOUT = 109,   //!< Brownout reset (software or hardware)
    RST_SDIO = 110,       //!< Reset over SDIO
} reset_reason_t;

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

class Hal {
    public:
        void restart();
        void restart(long pause);
        reset_reason_t resetReason();
        String resetReasonMessage();
        void enableHWWatchdog(long delaySeconds);
        void disableHWWatchdog();
        void feedHWWatchdog();
};

#endif
