#ifndef ONOFF_DETECTOR
#define ONOFF_DETECTOR

#include <Arduino.h>
#include "Console.h"
#include "IOConfig.h"
#include "ConfigPins.h"
#include "AbstractSensor.h"

#define MAX_ON_OFF_CHANNELS 8

class OnOffDetector : public AbstractSensor {    
    private: 
        Console *m_console;
        uint8_t m_registeredChannels = 0;
        String m_channelNames[MAX_ON_OFF_CHANNELS];
        uint8_t m_channels[MAX_ON_OFF_CHANNELS];
        bool m_pinStates[MAX_ON_OFF_CHANNELS];
        ConfigPins *m_configPins;

    public:
        OnOffDetector(Console *console, ConfigPins *configPins) {
            m_console = console;
            m_configPins = configPins;
        }

        void setup(IOConfig *config) { 
            configure(config);
        }

        void registerOnOffDetector(String name, uint8_t pin) {
            m_channelNames[m_registeredChannels] = name;
            m_channels[m_registeredChannels] = pin;

            pinMode(pin, INPUT);

            m_registeredChannels++;
        }

        void loop() {
            for(int idx = 0; idx < m_registeredChannels; ++idx) {
                m_pinStates[idx] = digitalRead(m_channels[idx]);
            }
        }

        void debugPrint(){
            for(int idx = 0; idx < m_registeredChannels; ++idx) {
                m_console->printVerbose(m_channelNames[idx] + "=" + (m_pinStates[idx] ? "on" : "off"));
            }
        }

        void configure(IOConfig *config){
            if(config->GPIO1Config == GPIO_CONFIG_INPUT) registerOnOffDetector(config->GPIO1Name, m_configPins->Gpio0);
            if(config->GPIO2Config == GPIO_CONFIG_INPUT) registerOnOffDetector(config->GPIO1Name, m_configPins->Gpio1);
            if(config->GPIO3Config == GPIO_CONFIG_INPUT) registerOnOffDetector(config->GPIO1Name, m_configPins->Gpio2);
            if(config->GPIO4Config == GPIO_CONFIG_INPUT) registerOnOffDetector(config->GPIO1Name, m_configPins->Gpio3);
            if(config->GPIO5Config == GPIO_CONFIG_INPUT) registerOnOffDetector(config->GPIO1Name, m_configPins->Gpio4);
            if(config->GPIO6Config == GPIO_CONFIG_INPUT) registerOnOffDetector(config->GPIO1Name, m_configPins->Gpio5);
            if(config->GPIO7Config == GPIO_CONFIG_INPUT) registerOnOffDetector(config->GPIO1Name, m_configPins->Gpio6);
            if(config->GPIO8Config == GPIO_CONFIG_INPUT) registerOnOffDetector(config->GPIO1Name, m_configPins->Gpio7);
        }

};

#endif