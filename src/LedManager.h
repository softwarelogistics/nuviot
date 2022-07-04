#ifndef LEDMANAGER_H
#define LEDMANAGER_H

#include "AbstractSensor.h"
#include "IOConfig.h"
#include <Arduino.h>
#include "Console.h"
#include "ConfigPins.h"

extern portMUX_TYPE timerMux;

extern volatile uint8_t beeperPin;
extern volatile uint8_t errPin;
extern volatile uint8_t onlinePin;

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

    void setup(IOConfig *config)
    {
        errPin = m_configPins->ErrorLED;
        onlinePin = m_configPins->OnlineLED;
        beeperPin = m_configPins->Buzzer;

        m_timer = timerBegin(0, 5, true);

        timerAttachInterrupt(m_timer, &onTimer, true);
        timerAlarmWrite(m_timer, 1000000, true);
        timerAlarmEnable(m_timer);

        if (errPin != -1)
            pinMode(m_configPins->ErrorLED, OUTPUT);
        if (onlinePin != -1)
            pinMode(m_configPins->OnlineLED, OUTPUT);
        if (m_configPins->Buzzer != -1)
            pinMode(m_configPins->Buzzer, OUTPUT);

        errPinState = HIGH;
        onlinePinState = HIGH;

        if (errPin != -1)
            digitalWrite(m_configPins->ErrorLED, errPinState);

        if (onlinePin != -1)
            digitalWrite(m_configPins->OnlineLED, onlinePinState);

        if (beeperPin != -1)
            digitalWrite(m_configPins->Buzzer, beeperPinState);

        errFlashRate = 0;
        beepRate = 0;
        onlineFlashRate = 4;
    }

    void loop()
    {
        if (isManualBeep && millis() > _beepComplete)
        {
            isManualBeep = false;
            _beepComplete = 0;
            if (beeperPin != -1)
                digitalWrite(beeperPin, LOW);
        }
    }

    void debugPrint()
    {
    }

    void setBeepRate(uint8_t rate)
    {
        beepRate = rate;
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