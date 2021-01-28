#ifndef ADC_H
#define ADC_H

#include <Wire.h>
#include "ADS1115.h"
#include "MessagePayload.h"
#include "AbstractSensor.h"
#include "Console.h"
#include "NuvIoTState.h"
#include "Display.h"
#include "IOConfig.h"
#include "ConfigPins.h"

#define NUMBER_ADC_PORTS 8

class ADC : public AbstractSensor
{
private:
    MessagePayload *m_messagePayload;
    ADS1115 *_bank1;
    ADS1115 *_bank2;
    Console *m_console;
    Display *m_display;
    NuvIoTState *m_state;
    bool m_portEnabled[NUMBER_ADC_PORTS];
    bool m_isCt[NUMBER_ADC_PORTS];
    float m_scalers[NUMBER_ADC_PORTS];
    String m_names[NUMBER_ADC_PORTS];

    bool m_bank1Enabled = false;
    bool m_bank2Enabled = false;

    ConfigPins *m_configPins;

public:
    ADC(TwoWire *wire, NuvIoTState *state, ConfigPins *configPins, Console *console, Display *display, MessagePayload *payload)
    {
        m_console = console;
        m_display = display;
        m_state = state;
        m_configPins = configPins;

        /* addr2 = +5V, ADCMOD1 */
        _bank1 = new ADS1115(wire, ADS1115_ADDRESS2, 1);

        /* addr1 = GND, ADCMOD2 */
        _bank2 = new ADS1115(wire, ADS1115_ADDRESS1, 2);

        for (int idx = 0; idx < NUMBER_ADC_PORTS; ++idx)
        {
            m_portEnabled[idx] = false;
            m_isCt[idx] = false;
        }

        m_messagePayload = payload;

        /* 
         *
         * Bank 1 
         * =================
         * idx: LBL  - Connector
         * 0 = VBATT - POWER
         * 1 = ADC2  - ADC3
         * 2 = ADC3  - CT 3 ADC6
         * 3 = ADC4  - N/C
         *
         *  
         * Bank 2
         * =================
         * 4 = ADC6  - CT 2 ADC5
         * 5 = ADC5  - CT 1 ADC4
         * 6 = ADC7  - ADC 2
         * 7 = ADC8  - ADC 1
         * 
         */

        for (int idx = 0; idx < NUMBER_ADC_PORTS; ++idx)
        {
            m_scalers[idx] = 1.0;
        }
    }

    void setScaler(uint8_t channel, float scaler)
    {
        m_scalers[channel] = scaler;
    }

    void setConvesionDelay(uint8_t conversionDelay)
    {
        _bank1->setConversionDelay(conversionDelay);
        _bank2->setConversionDelay(conversionDelay);
    }

    ~ADC()
    {
        delete _bank1;
        _bank1 = NULL;
        delete _bank2;
        _bank2 = NULL;
    }

    float getADC(byte port)
    {
        if (port > 7)
        {
            m_console->printError("ADC Port > 7");
        }

        switch (port)
        {
        case 0:
            return m_portEnabled[port] ? m_messagePayload->voltage1 : -1;
        case 1:
            return m_portEnabled[port] ? m_messagePayload->voltage2 : -1;
        case 2:
            return m_portEnabled[port] ? m_messagePayload->voltage3 : -1;
        case 3:
            return m_portEnabled[port] ? m_messagePayload->voltage4 : -1;
        case 4:
            return m_portEnabled[port] ? m_messagePayload->voltage5 : -1;
        case 5:
            return m_portEnabled[port] ? m_messagePayload->voltage6 : -1;
        case 6:
            return m_portEnabled[port] ? m_messagePayload->voltage7 : -1;
        case 7:
            return m_portEnabled[port] ? m_messagePayload->voltage8 : -1;
        }

        return -1;
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
            if (m_bank2Enabled)
            {
                result = _bank2->readADC_Voltage(channel - 4);
            }
            else
            {
                bool toggle = false;
                while (1)
                {
                    m_display->clearBuffer();
                    m_display->setTextSize(2);
                    toggle = !toggle;
                    if (toggle)
                        m_display->drawStr("ERROR", "ADC BANK2", "NOT ENABLED", "!!!!");
                    else
                        m_display->drawStr("ERROR", "ADC BANK2", "NOT ENABLED");
                    m_display->sendBuffer();
                    m_console->printError("adcbank1=notenabled; // Call to read voltage on disabled bank 2 channel " + String(channel));
                    m_state->loop();
                    delay(1000);
                };
            }
        }
        else
        {
            if(m_bank1Enabled)
            {
            result = _bank1->readADC_Voltage(channel);
            }
            else
            {                  
                bool toggle = false;
                while (1)
                {
                    m_display->clearBuffer();
                    m_display->setTextSize(2);
                    toggle = !toggle;
                    if (toggle)
                        m_display->drawStr("ERROR", "ADC BANK1", "NOT ENABLED", "!!!!");
                    else
                        m_display->drawStr("ERROR", "ADC BANK1", "NOT ENABLED");
                    m_display->sendBuffer();
                    m_console->printError("adcbank1=notready; // Call to read voltage on disabled bank 1 channel " + String(channel));
                    m_state->loop();
                    delay(1000);
                };            
            }
        }

        return result;
    }

    void enableADC(String name, int index, bool enabled)
    {
        if (index > 7)
        {
            m_console->printError("ADC > 7");
        }
        
        if (enabled)
        {
            m_console->println("adc" + String(index) + "=enabled, adcbank=" + String(index > 3 ? 2 : 1) + ";");
        }

        m_portEnabled[index] = enabled;
        m_names[index] = name;
    }

    void enableADCAsCT(String name, int index, bool enabled)
    {
        if (index > 7)
        {
            m_console->printError("ADC > 7");
        }
        
        if (enabled)
        {
            m_console->println("adc" + String(index) + "=enabledAsCT, adcbank=" + String(index > 3 ? 2 : 1) + ";");
        }

        m_portEnabled[index] = enabled;
        m_names[index] = name;
        m_isCt[index] = true;
    }

    void setup(IOConfig *ioConfig)
    {
        configure(ioConfig);
    }

    void loop()
    {
        if (!_bank1->isOnline() && m_bank1Enabled)
        {
            m_messagePayload->lastError = "ADC 1 Offline";
            m_console->printError("ADC 1 Offline");
            m_messagePayload->status = "Error";
        }
        else if (!_bank2->isOnline() && m_bank2Enabled)
        {
            m_messagePayload->lastError = "ADC 2 Offline";
            m_console->printError("ADC 2 Offline");
            m_messagePayload->status = "Error";
        }

        m_messagePayload->hasVoltage1 = m_portEnabled[0];
        m_messagePayload->hasVoltage2 = m_portEnabled[1];
        m_messagePayload->hasVoltage3 = m_portEnabled[2];
        m_messagePayload->hasVoltage4 = m_portEnabled[3];
        m_messagePayload->hasVoltage5 = m_portEnabled[4];
        m_messagePayload->hasVoltage6 = m_portEnabled[5];
        m_messagePayload->hasVoltage7 = m_portEnabled[6];
        m_messagePayload->hasVoltage8 = m_portEnabled[7];

        m_messagePayload->voltage1 = m_messagePayload->hasVoltage1 ? getVoltage(m_configPins->ADCChannel1) * m_scalers[0] : -1; // 0
        m_messagePayload->voltage2 = m_messagePayload->hasVoltage2 ? getVoltage(m_configPins->ADCChannel2) * m_scalers[1] : -1; // 1
        m_messagePayload->voltage3 = m_messagePayload->hasVoltage3 ? getVoltage(m_configPins->ADCChannel3) * m_scalers[2] : -1; // 2
        m_messagePayload->voltage4 = m_messagePayload->hasVoltage4 ? getVoltage(m_configPins->ADCChannel4) * m_scalers[3] : -1; // 3
        m_messagePayload->voltage5 = m_messagePayload->hasVoltage5 ? getVoltage(m_configPins->ADCChannel5) * m_scalers[4] : -1; // 4
        m_messagePayload->voltage6 = m_messagePayload->hasVoltage6 ? getVoltage(m_configPins->ADCChannel6) * m_scalers[5] : -1; // 5
        m_messagePayload->voltage7 = m_messagePayload->hasVoltage7 ? getVoltage(m_configPins->ADCChannel7) * m_scalers[6] : -1; // 6
        m_messagePayload->voltage8 = m_messagePayload->hasVoltage8 ? getVoltage(m_configPins->ADCChannel8) * m_scalers[7] : -1; // 7
    }

    void configure(IOConfig *ioConfig)
    {
        enableADC(ioConfig->ADC1Name, 0, ioConfig->ADC1Config == ADC_CONFIG_ADC);
        enableADC(ioConfig->ADC2Name, 1, ioConfig->ADC2Config == ADC_CONFIG_ADC);
        enableADC(ioConfig->ADC3Name, 2, ioConfig->ADC3Config == ADC_CONFIG_ADC);
        enableADC(ioConfig->ADC4Name, 3, ioConfig->ADC4Config == ADC_CONFIG_ADC);
        enableADC(ioConfig->ADC5Name, 4, ioConfig->ADC5Config == ADC_CONFIG_ADC);
        enableADC(ioConfig->ADC6Name, 5, ioConfig->ADC6Config == ADC_CONFIG_ADC);
        enableADC(ioConfig->ADC7Name, 6, ioConfig->ADC7Config == ADC_CONFIG_ADC);
        enableADC(ioConfig->ADC8Name, 7, ioConfig->ADC8Config == ADC_CONFIG_ADC);

        setScaler(0, ioConfig->ADC1Scaler);
        setScaler(1, ioConfig->ADC2Scaler);
        setScaler(2, ioConfig->ADC3Scaler);
        setScaler(3, ioConfig->ADC4Scaler);
        setScaler(4, ioConfig->ADC5Scaler);
        setScaler(5, ioConfig->ADC6Scaler);
        setScaler(6, ioConfig->ADC7Scaler);
        setScaler(7, ioConfig->ADC8Scaler);
    }

    void debugPrint()
    {
        for (int idx = 0; idx < NUMBER_ADC_PORTS; ++idx)
        {
            if (m_portEnabled[idx] && !m_isCt[idx])
            {
                m_console->printVerbose(m_names[idx] + "=" + String(getADC(idx)) + ";");
            }
        }
    }

    /**
     * \brief Enable ADC Bank
     * 
     * Each board has two ADC converters, each ADC Converter has 4 channels. 
     * 
     * In some cases you may not need 8 ADC channels, if that's the case
     * then you don't install the ADC bank and not enable it.
     * 
     * \param bank Index of bank to initialized (1 or 2)
     * \param enabled True to enable, false to disable.
     * 
     */
    bool setBankEnabled(int bank, bool enabled)
    {
        switch (bank)
        {
        case 1:
            m_bank1Enabled = enabled;
            break;
        case 2:
            m_bank2Enabled = enabled;
            break;
        }

        return true;
    }

    /**
     * \brief Determine if bank one is online.
     * 
     * This method can be called to make an I2C call
     * to the ADC on bank one, if it finds it, the method
     * will return true otherewise it return false.
     * 
     * \return True if the ADC respondes, false if not.
     **/
    bool isBankOneOnline()
    {
        return _bank1->isOnline();
    }

    bool isBank1Enabled()
    {
        return m_bank1Enabled;
    }

    /**
     * \brief Determine if bank two is online.
     * 
     * This method can be called to make an I2C call
     * to the ADC on bank two, if it finds it, the method
     * will return true otherewise it return false.
     * 
     * \return True if the ADC respondes, false if not.
     **/
    bool isBankTwoOnline()
    {
        return _bank2->isOnline();
    }

    bool isBank2Enabled()
    {
        return m_bank2Enabled;
    }
};
#endif