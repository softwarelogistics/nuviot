#ifndef ADC_H
#define ADC_H

#include "ADS1115.h"
#include <Wire.h>
#include <Logger.h>

class ADC
{
private:
    Logger *m_logger;
    ADS1115 *_bank1;
    ADS1115 *_bank2;

public:
    ADC(TwoWire *wire, Logger *logger)
    {
        /* addr2 = +5V, ADCMOD1 */
        _bank1 = new ADS1115(wire, ADS1115_ADDRESS2);

        /* addr1 = GND, ADCMOD2 */
        _bank2 = new ADS1115(wire, ADS1115_ADDRESS1);

        /* 
         *
         * Bank 1 
         * =================
         * idx: LBL  - Connector
         * 0 = VBATT - ADC0
         * 1 = ADC2  - ADC3
         * 2 = ADC3  - CT 3
         * 3 = ADC4  - N/C
         *
         *  
         * Bank 2
         * =================
         * 4 = ADC6  - CT 2 
         * 5 = ADC5  - CT 1
         * 6 = ADC7  - ADC 2
         * 7 = ADC8  - ADC 1
         * 
         */
    }

    ~ADC()
    {
        delete _bank1;
        _bank1 = NULL;
        delete _bank2;
        _bank2 = NULL;
    }

    float getVoltage(uint8_t channel)
    {
        if (channel < 0)
            return -1;

        if (channel > 7)
        {
            return -1;
        }

        if (channel > 3)
            return _bank2->readADC_Voltage(channel - 4);

        return _bank1->readADC_Voltage(channel);
    }

    float getVBATT()
    {
        return getVoltage(0);
    }

    float getADC1()
    {
        return getVoltage(7);
    }

    float getADC2()
    {
        return getVoltage(6);
    }

    float getADC3()
    {
        return getVoltage(1);
    }
};
#endif