#ifndef ADC_H
#define ADC_H

#include <Wire.h>
#include "ADS1115.h"
#include "Logger.h"
#include "MessagePayload.h"
#include "AbstractSensor.h"


class ADC : public AbstractSensor
{
private:
    Logger *m_logger;
    MessagePayload *m_messagePayload;
    ADS1115 *_bank1;
    ADS1115 *_bank2;
    bool m_portEnabled[3];
    bool m_vbattEnabled;

    bool m_bank1Enabled = false;
    bool m_bank2Enabled = false;

public:
    ADC(TwoWire *wire, MessagePayload *payload, Logger *logger)
    {
        /* addr2 = +5V, ADCMOD1 */
        _bank1 = new ADS1115(wire, ADS1115_ADDRESS2);

        /* addr1 = GND, ADCMOD2 */
        _bank2 = new ADS1115(wire, ADS1115_ADDRESS1);

        m_portEnabled[0] = false;
        m_portEnabled[1] = false;
        m_portEnabled[2] = false;
        m_vbattEnabled = false;
        m_messagePayload = payload;

        /* 
         *
         * Bank 1 
         * =================
         * idx: LBL  - Connector
         * 0 = VBATT - POWER
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

        float result = 0.0;

        if (channel > 3)
        {
            result = _bank2->readADC_Voltage(channel - 4);
        }
        else
        {
            result = _bank1->readADC_Voltage(channel);
        }

        return result;
    }

    float getVBATT()
    {
        return (m_vbattEnabled) ? getVoltage(0) : -1;
    }

    float getADC(byte port)
    {
        if (port > 2)
        {
            m_logger->logError("Attempt to get ADC value for port 3 or greater, only adc ports 0, 1 and 2 are valid.");
        }

        switch (port)
        {
        case 0:
            return m_portEnabled[port] ? m_messagePayload->voltage1 : -1;
        case 1:
            return m_portEnabled[port] ? m_messagePayload->voltage2 : -1;
        case 2:
            return m_portEnabled[port] ? m_messagePayload->voltage3 : -1;
        }

        return -1;
    }

    void enableVATT(bool enabled)
    {
        m_vbattEnabled = true;
    }

    void enableADC(int port, bool enabled)
    {
        if (port > 2)
        {
            m_logger->logError("Attempt to enable ADC port 3 or greater, only adc ports 0, 1 and 2 are valid.");
        }

        m_portEnabled[port] = enabled;
    }

    void setup()
    {

    }

    void loop()
    {
        if(!_bank1->isOnline() && m_bank1Enabled) {
            m_messagePayload->lastError = "ADC Bank 1 Offline";
            m_logger->logError("ADC Bank 1 Offline");
            m_messagePayload->status = "Error";
        }
        else if(!_bank2->isOnline() && m_bank2Enabled) {
            m_messagePayload->lastError = "ADC Bank 2 Offline";
            m_logger->logError("ADC Bank 2 Offline");
            m_messagePayload->status = "Error";
        }

        _bank2->isOnline();

        m_messagePayload->hasVBatt = m_vbattEnabled;
        
        m_messagePayload->hasVoltage1 = m_portEnabled[0];
        m_messagePayload->hasVoltage2 = m_portEnabled[1];        
        m_messagePayload->hasVoltage3 = m_portEnabled[2];

        m_messagePayload->vbatt = m_vbattEnabled ? getVBATT() : -1;
        delay(250);
        m_messagePayload->voltage1 = m_messagePayload->hasVoltage1 ? getVoltage(7) : -1;
        delay(250);
        m_messagePayload->voltage2 = m_messagePayload->hasVoltage2 ? getVoltage(6) : -1;
        delay(250);
        m_messagePayload->voltage3 = m_messagePayload->hasVoltage3 ? getVoltage(1) : -1;
        delay(250);
    }

    void debugPrint() {
        if(m_vbattEnabled) m_logger->logVerbose("VBATT  : " + String(getVBATT()));
        for(int idx = 0; idx < 3; ++idx)
            if(m_portEnabled[idx])
                m_logger->logVerbose("VADC" + String(idx) + "  :" + String(getADC(idx)));
    }

    bool setBankEnabled(int bank, bool enabled)
    {
        switch(bank) {
            case 1: m_bank1Enabled = enabled; break;
            case 2: m_bank2Enabled = enabled; break;
        }
    }

    bool isBankOneOnline()
    {
        return _bank1->isOnline();
    }

    bool isBankTwoOnline()
    {
        return _bank2->isOnline();
    }
};
#endif