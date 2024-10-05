#ifndef PIR_H
#define PIR_H

#include <Arduino.h>
#include "Console.h"
#include "ADC.h"
#include "IOConfig.h"
#include "ConfigPins.h"
#include "MessagePayload.h"
#include "AbstractSensor.h"

class PIRSensor: public AbstractSensor {
private: 
    Console *m_console;

    float m_threshold[MAX_ON_OFF_CHANNELS];
    String m_channelNames[MAX_ON_OFF_CHANNELS];
    uint8_t m_channelIndex[MAX_ON_OFF_CHANNELS];
    uint8_t m_channels[MAX_ON_OFF_CHANNELS];

    bool m_pinStates[MAX_ON_OFF_CHANNELS];
    uint8_t m_registeredChannels = 0;

    ADC *m_adc;
    ConfigPins *m_configPins;
    MessagePayload *m_payload;

    public:
     PIRSensor(ADC *adc, Console *console, ConfigPins *configPins, MessagePayload *payload) {
        m_console = console;
        m_configPins = configPins;
        m_payload = payload;
        m_adc = adc;
    }

    void registerPIRDetector(String name, uint8_t pin, float threshold, uint8_t idx){
        m_channelNames[m_registeredChannels] = name;
        m_channels[m_registeredChannels] = pin;
        m_channelIndex[m_registeredChannels] = idx;
        m_threshold[m_registeredChannels] = threshold;

        pinMode(pin, ANALOG);

        m_console->println("pir=configure; // " + name + " port: " + String(idx) + " pin: " + String(pin));

        m_registeredChannels++;
    }    

    void configure(IOConfig *config){
        setup(config);
    }

    void setup(IOConfig *config){
        if (config->ADC1Config == ADC_CONFIG_PIR)
            registerPIRDetector(config->ADC1Name, m_configPins->ADCChannel1, config->ADC1Zero, 1);
        if (config->ADC2Config == ADC_CONFIG_PIR)
            registerPIRDetector(config->ADC2Name, m_configPins->ADCChannel2, config->ADC2Zero, 2);
        if (config->ADC3Config == ADC_CONFIG_PIR)
            registerPIRDetector(config->ADC3Name, m_configPins->ADCChannel3, config->ADC3Zero, 3);
        if (config->ADC4Config == ADC_CONFIG_PIR)
            registerPIRDetector(config->ADC4Name, m_configPins->ADCChannel4, config->ADC4Zero, 4);
        if (config->ADC5Config == ADC_CONFIG_PIR)
            registerPIRDetector(config->ADC5Name, m_configPins->ADCChannel5, config->ADC5Zero, 5);
        if (config->ADC6Config == ADC_CONFIG_PIR)
            registerPIRDetector(config->ADC6Name, m_configPins->ADCChannel6, config->ADC6Zero, 6);
        if (config->ADC7Config == ADC_CONFIG_PIR)
            registerPIRDetector(config->ADC7Name, m_configPins->ADCChannel7, config->ADC7Zero, 7);
        if (config->ADC8Config == ADC_CONFIG_PIR)
            registerPIRDetector(config->ADC8Name, m_configPins->ADCChannel8, config->ADC8Zero, 8);
    }

    void loop() {
        for(uint8_t idx = 0; idx < m_registeredChannels; ++idx) {
            // todo: we should really read this from ADC
            uint16_t counts = analogRead(m_channels[idx]);
            if(counts > m_threshold[idx] && !m_pinStates[idx]) {                
                m_payload->ioValues->setValue(m_channelIndex[idx], 1);
                m_console->println("[PIRSensor__Loop] Motion detected on channel:"  + m_channelNames[idx]);
                m_pinStates[idx] = true;
            }
        }        
    }

    void clearMotion() {
        for(uint8_t idx = 0; idx < m_registeredChannels; ++idx) {
            if(m_pinStates[idx])
                m_console->println("[PIRSensor__Loop] Motion cleared on channel:"  + m_channelNames[idx]);
                
            m_pinStates[idx] = false;
            m_payload->ioValues->setValue(m_channelIndex[idx], 0);
        }
    }

    void debugPrint() {
        for(uint8_t idx = 0; idx < m_registeredChannels; ++idx) {
            m_console->printVerbose("PIR: " + m_channelNames[idx] + " = " + String(m_pinStates[idx]));
        }
    }
};

#endif 