#ifndef LEDMANAGER_H
#define LEDMANAGER_H

#include "AbstractSensor.h"
#include "IOConfig.h"
#include <Arduino.h>
#include "Console.h"
#include "ConfigPins.h"

extern portMUX_TYPE timerMux;

extern volatile int8_t LED_ON_STATE;
extern volatile int8_t LED_OFF_STATE;

extern volatile int8_t beeperPin;
extern volatile int8_t errPin;
extern volatile int8_t onlinePin;

extern volatile int8_t beepRate;
extern volatile int8_t errFlashRate;
extern volatile int8_t onlineFlashRate;

extern volatile uint8_t beeperCountDown;
extern volatile uint8_t errFlashCountDown;
extern volatile uint8_t onlineFlashCountDown;

extern volatile bool isManualBeep;

extern volatile uint8_t beeperPinState;
extern volatile uint8_t errPinState;
extern volatile uint8_t onlinePinState;

extern void IRAM_ATTR onTimer();

class LedManager : public AbstractSensor
{
private:
    Console *m_console;
    ConfigPins *m_configPins;
    hw_timer_t *m_timer = NULL;
    long _beepComplete;

    volatile uint32_t interruptCounter;

public:
    LedManager(Console *console, ConfigPins *configPins)
    {
        m_console = console;
        m_configPins = configPins;
    }

    void beep(int delayMS)
    {
        isManualBeep = true;
        _beepComplete = millis() + delayMS;
        digitalWrite(beeperPin, HIGH);
    }

    void setup(IOConfig *config);
    void loop();
    void debugPrint(){}
    void setBeepRate(uint8_t rate){beepRate = rate;}
    void setErrFlashRate(uint8_t rate){errFlashRate = rate;}
    void setOnlineFlashRate(uint8_t rate){onlineFlashRate = rate;}
};

#endif