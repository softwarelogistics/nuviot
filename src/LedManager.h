#ifndef LEDMANAGER_H
#define LEDMANAGER_H

#include "AbstractSensor.h"
#include "IOConfig.h"
#include <Arduino.h>
#include "Console.h"
#include "ConfigPins.h"

extern portMUX_TYPE timerMux;

extern volatile uint8_t errPin;
extern volatile uint8_t onlinePin;

extern volatile int8_t errFlashRate;
extern volatile int8_t onlineFlashRate;

extern volatile uint8_t errFlashCountDown;
extern volatile uint8_t onlineFlashCountDown;

extern volatile uint8_t errPinState;
extern volatile uint8_t onlinePinState;

extern void IRAM_ATTR onTimer();

class LedManager : public AbstractSensor
{
private:
    IOConfig *m_ioConfig;
    Console *m_console;
    ConfigPins *m_configPins;
    hw_timer_t *m_timer = NULL;

    volatile uint32_t interruptCounter;

public:
    LedManager(Console *console, ConfigPins *configPins)
    {
        m_console = console;
        m_configPins = configPins;
    }

    void setup(IOConfig *ioConfig)
    {
        m_ioConfig = ioConfig;

        errPin = m_configPins->ErrorLED;
        onlinePin = m_configPins->OnlineLED;

        m_timer = timerBegin(0, 5, true);

        timerAttachInterrupt(m_timer, &onTimer, true);
        timerAlarmWrite(m_timer, 1000000, true);
        timerAlarmEnable(m_timer);

        pinMode(m_configPins->ErrorLED, OUTPUT);
        pinMode(m_configPins->OnlineLED, OUTPUT);

        errPinState = HIGH;
        onlinePinState = HIGH;

        digitalWrite(m_configPins->ErrorLED, errPinState);
        digitalWrite(m_configPins->OnlineLED, onlinePinState);

        errFlashRate = 0;
        onlineFlashRate = 4;
    }

    void loop()
    {
    }

    void debugPrint()
    {
    }

    void setErrFlashRate(uint8_t rate)
    {
        errFlashRate = rate;
    }

    void setOnlineFlashRate(uint8_t rate)
    {
        onlineFlashRate = rate;
    }
};

#endif