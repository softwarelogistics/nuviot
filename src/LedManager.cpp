#include "LedManager.h"

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile int8_t invertLED = true;
volatile int8_t beeperPin = -1;
volatile int8_t errPin = -1;
volatile int8_t onlinePin = -1;

volatile int8_t LED_ON_STATE;
volatile int8_t LED_OFF_STATE;

volatile int8_t beepRate = 0;
volatile int8_t errFlashRate = 0;
volatile int8_t onlineFlashRate = 0;

volatile uint8_t beeperCountDown = 0;
volatile uint8_t errFlashCountDown = 0;
volatile uint8_t onlineFlashCountDown = 0;

volatile uint8_t beeperPinState = 1;
volatile uint8_t errPinState = 1;
volatile uint8_t onlinePinState = 1;
;
volatile bool isManualBeep = false;

void IRAM_ATTR onTimer()
{
    portENTER_CRITICAL_ISR(&timerMux);
    if (errPin != -1)
    {
        if (errFlashRate == 0)
        {
            if (errPinState != LED_OFF_STATE)
            {
                digitalWrite(errPin, LED_OFF_STATE);
                errPinState = LED_OFF_STATE;
            }
        }
        else if (errFlashRate == -1)
        {
            if (errPinState != LED_ON_STATE)
            {
                digitalWrite(errPin, LED_ON_STATE);
                errPinState = LED_ON_STATE;
            }
        }
        else if (errFlashCountDown-- <= 0)
        {
            errFlashCountDown = errFlashRate;
            errPinState = errPinState == LED_ON_STATE ? LED_OFF_STATE : LED_ON_STATE;
            digitalWrite(errPin, errPinState);
        }
    }

    if (beeperPin != -1)
    {
        if (!isManualBeep)
        {
            if (beepRate == 0)
            {
                if (beeperPinState != LED_OFF_STATE)
                {
                    digitalWrite(beeperPin, LED_OFF_STATE);
                    beeperPinState = LED_OFF_STATE;
                }
            }
            else if (beepRate == -1)
            {
                if (beeperPinState != LED_ON_STATE)
                {
                    digitalWrite(beeperPin, LED_ON_STATE);
                    beeperPinState = LED_ON_STATE;
                }
            }
            else if (beeperCountDown-- <= 0)
            {
                beeperCountDown = beepRate;
                beeperPinState = beeperPinState == LED_ON_STATE ? LED_OFF_STATE : LED_ON_STATE;
                digitalWrite(beeperPin, beeperPinState);
            }
        }
    }

    if (onlinePin != -1)
    {
        if (onlineFlashRate == 0)
        {
            if (onlinePinState != LED_OFF_STATE)
            {
                digitalWrite(onlinePin, 1);
                onlinePinState = LED_OFF_STATE;
            }
        }
        else if (onlineFlashRate == -1)
        {
            if (onlinePinState != LED_ON_STATE)
            {
                digitalWrite(onlinePin, LED_ON_STATE);
                onlinePinState = LED_ON_STATE;
            }
        }
        else if (onlineFlashCountDown-- <= 0)
        {
            onlineFlashCountDown = onlineFlashRate;
            onlinePinState = onlinePinState == LED_OFF_STATE ? LED_ON_STATE : LED_OFF_STATE;
            digitalWrite(onlinePin, onlinePinState);
        }
    }

    portEXIT_CRITICAL_ISR(&timerMux);
}

void LedManager::setup(IOConfig *config)
{
    errPin = m_configPins->ErrorLED;
    onlinePin = m_configPins->OnlineLED;
    beeperPin = m_configPins->Buzzer;

    if (m_configPins->InvertLED)
    {
        LED_ON_STATE = LOW;
        LED_OFF_STATE = HIGH;
    }
    else
    {
        LED_ON_STATE = HIGH;
        LED_OFF_STATE = LOW;
    }

    m_timer = timerBegin(0, 5, true);

    timerAttachInterrupt(m_timer, &onTimer, true);
    timerAlarmWrite(m_timer, 1000000, true);
    timerAlarmEnable(m_timer);

    if (errPin != -1)
        pinMode(m_configPins->ErrorLED, OUTPUT);
    if (onlinePin != -1)
        pinMode(m_configPins->OnlineLED, OUTPUT);
    if (beeperPin != -1)
        pinMode(m_configPins->Buzzer, OUTPUT);

    errPinState = LED_OFF_STATE;
    onlinePinState = LED_OFF_STATE;

    if (errPin != -1)
        digitalWrite(errPin, errPinState);

    if (onlinePin != -1)
        digitalWrite(onlinePin, onlinePinState);

    if (beeperPin != -1)
        digitalWrite(beeperPin, beeperPinState);

    errFlashRate = 0;
    beepRate = 0;
    onlineFlashRate = 4;
}

void LedManager::loop()
{
    if (isManualBeep && millis() > _beepComplete)
    {
        isManualBeep = false;
        _beepComplete = 0;
        if (beeperPin != -1)
            digitalWrite(beeperPin, LOW);
    }
}