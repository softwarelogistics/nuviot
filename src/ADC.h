#ifndef ADC_H
#define ADC_H

#include <Wire.h>
#include "ADS1115.h"
#include "IOValues.h"
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
    IOValues *m_ioValues;
    ADS1115 *_bank1;
    ADS1115 *_bank2;
    Console *m_console;
    Display *m_display;
    NuvIoTState *m_state;
    MessagePayload *m_payload;
    bool m_portEnabled[NUMBER_ADC_PORTS];
    bool m_isCt[NUMBER_ADC_PORTS];
    float m_calibration[NUMBER_ADC_PORTS];
    float m_scalers[NUMBER_ADC_PORTS];
    float m_zero[NUMBER_ADC_PORTS];
    float m_rawValues[NUMBER_ADC_PORTS];
    uint8_t m_pins[NUMBER_ADC_PORTS];
    String m_names[NUMBER_ADC_PORTS];

    bool m_bank1Enabled = false;
    bool m_bank2Enabled = false;

public:
    ADC(TwoWire *wire, NuvIoTState *state, ConfigPins *configPins, Console *console, Display *display, MessagePayload *payload)
    {
        m_console = console;
        m_display = display;
        m_state = state;
        m_payload = payload;

        m_pins[0] = configPins->ADCChannel1;
        m_pins[1] = configPins->ADCChannel2;
        m_pins[2] = configPins->ADCChannel3;
        m_pins[3] = configPins->ADCChannel4;
        m_pins[4] = configPins->ADCChannel5;
        m_pins[5] = configPins->ADCChannel6;
        m_pins[6] = configPins->ADCChannel7;
        m_pins[7] = configPins->ADCChannel8;

        /* addr2 = +5V, ADCMOD1 */
        _bank1 = new ADS1115(wire, ADS1115_ADDRESS2, 1);

        /* addr1 = GND, ADCMOD2 */
        _bank2 = new ADS1115(wire, ADS1115_ADDRESS1, 2);

        for (int idx = 0; idx < NUMBER_ADC_PORTS; ++idx)
        {
            m_portEnabled[idx] = false;
            m_isCt[idx] = false;
        }

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

    void setZero(uint8_t channel, float zero)
    {
        m_zero[channel] = zero;
    }

    void setCalibration(uint8_t channel, float calibration)
    {
        m_calibration[channel] = calibration;
    }

    void setConversionDelay(uint8_t conversionDelay)
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

    float getRawVoltage(byte port)
    {
        if (port > 7)
        {
            m_console->printError("ADC Port > 7");
            return -1;
        }

        if (m_portEnabled[port])
        {
            return m_rawValues[port];
        }
        else
        {
            return -1;
        }
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
            if (m_bank1Enabled)
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

    void applyValues()
    {
        for (int idx = 0; idx < NUMBER_ADC_PORTS; ++idx)
        {
            if (m_portEnabled[idx])
            {
                m_rawValues[idx] = ((m_rawValues[idx] * m_calibration[idx]) - m_zero[idx]) * m_scalers[idx];
                m_payload->ioValues->setValue(idx, m_rawValues[idx]);
            }
            else
            {
                m_rawValues[idx] = -1;
                m_payload->ioValues->clearValue(idx);
            }
        }
    }

    void loop(double values[])
    {
        for (int idx = 0; idx < NUMBER_ADC_PORTS; ++idx)
        {
            m_rawValues[idx] = values[idx];
        }

        applyValues();
    }

    void loop()
    {
        if (!_bank1->isOnline() && m_bank1Enabled)
        {
            m_payload->lastError = "ADC 1 Offline";
            m_console->printError("adc1=offline;");
            m_payload->status = "Error";
        }
        else if (!_bank2->isOnline() && m_bank2Enabled)
        {
            m_payload->lastError = "ADC 2 Offline";
            m_console->printError("adc2=offline;");
            m_payload->status = "Error";
        }

        for (int idx = 0; idx < NUMBER_ADC_PORTS; ++idx)
        {
            if (m_portEnabled[idx])
            {
                m_rawValues[idx] = getVoltage(m_pins[idx]);
            }
        }

        applyValues();
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

        setZero(0, ioConfig->ADC1Zero);
        setZero(1, ioConfig->ADC2Zero);
        setZero(2, ioConfig->ADC3Zero);
        setZero(3, ioConfig->ADC4Zero);
        setZero(4, ioConfig->ADC5Zero);
        setZero(5, ioConfig->ADC6Zero);
        setZero(6, ioConfig->ADC7Zero);
        setZero(7, ioConfig->ADC8Zero);

        setCalibration(0, ioConfig->ADC1Calibration);
        setCalibration(1, ioConfig->ADC2Calibration);
        setCalibration(2, ioConfig->ADC3Calibration);
        setCalibration(3, ioConfig->ADC4Calibration);
        setCalibration(4, ioConfig->ADC5Calibration);
        setCalibration(5, ioConfig->ADC6Calibration);
        setCalibration(6, ioConfig->ADC7Calibration);
        setCalibration(7, ioConfig->ADC8Calibration);
    }

    void debugPrint()
    {
        for (int idx = 0; idx < NUMBER_ADC_PORTS; ++idx)
        {
            if (m_portEnabled[idx] && !m_isCt[idx])
            {
                m_console->printVerbose(m_names[idx] + "=" + String(m_rawValues[idx]) + ";");
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