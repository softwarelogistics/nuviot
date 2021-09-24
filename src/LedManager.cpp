#include "LedManager.h"


portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint8_t beeperPin = -1;
volatile uint8_t errPin = -1;
volatile uint8_t onlinePin = -1;

volatile int8_t beepRate = 0;
volatile int8_t errFlashRate = 0;
volatile int8_t onlineFlashRate = 0;

volatile uint8_t beeperCountDown = 0;
volatile uint8_t errFlashCountDown = 0;
volatile uint8_t onlineFlashCountDown = 0;

volatile uint8_t beeperPinState = 1;
volatile uint8_t errPinState = 1;
volatile uint8_t onlinePinState = 1;;
volatile bool isManualBeep = false;

void IRAM_ATTR onTimer()
{
    portENTER_CRITICAL_ISR(&timerMux);
    if (errFlashRate == 0)
    {
        if (errPinState != HIGH)
        {
            digitalWrite(errPin, HIGH);
            errPinState = HIGH;
        }
    }
    else if (errFlashRate == -1)
    {
        if (errPinState != LOW)
        {
            digitalWrite(errPin, LOW);
            errPinState = LOW;
        }
    }    
    else if (errFlashCountDown-- <= 0)
    {
        errFlashCountDown = errFlashRate;
        errPinState = errPinState == HIGH ? LOW : HIGH;
        digitalWrite(errPin, errPinState);
    }

    if(!isManualBeep){
        if(beepRate == 0)
        {
            if (beeperPinState != LOW)
            {
                digitalWrite(beeperPin, LOW);
                beeperPinState = LOW;
            }
        }
        else if(beepRate == -1)
        {
            if(beeperPinState != HIGH)
            {
                digitalWrite(beeperPin, HIGH);
                beeperPinState = HIGH;
            }
        }    
        else if (beeperCountDown-- <= 0)
        {
            beeperCountDown = beepRate;
            beeperPinState = beeperPinState == HIGH ? LOW : HIGH;
            digitalWrite(beeperPin, beeperPinState);
        }
    }

    if (onlineFlashRate == 0)
    {
        if (onlinePinState != HIGH)
        {
            digitalWrite(onlinePin, 1);
            onlinePinState = HIGH;
        }
    }
    else if (onlineFlashRate == -1)
    {
        if (onlinePinState != LOW)
        {
            digitalWrite(onlinePin, LOW);
            onlinePinState = LOW;
        }
    }    
    else if (onlineFlashCountDown-- <= 0)
    {
        onlineFlashCountDown = onlineFlashRate;
        onlinePinState = onlinePinState == HIGH ? LOW : HIGH;
        digitalWrite(onlinePin, onlinePinState);
    }

    portEXIT_CRITICAL_ISR(&timerMux);
}
