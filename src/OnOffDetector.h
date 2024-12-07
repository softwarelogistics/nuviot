#ifndef ONOFF_DETECTOR
#define ONOFF_DETECTOR

#include <Arduino.h>
#include "Console.h"
#include "IOConfig.h"
#include "ConfigPins.h"
#include "AbstractSensor.h"
#include "MessagePayload.h"

#define MAX_ON_OFF_CHANNELS 8

class OnOffDetector : public AbstractSensor
{
private:
    Console *m_console;
    uint8_t m_registeredChannels = 0;

    bool m_invert[MAX_ON_OFF_CHANNELS];
    String m_channelNames[MAX_ON_OFF_CHANNELS];
    uint8_t m_channelIndex[MAX_ON_OFF_CHANNELS];
    uint8_t m_channels[MAX_ON_OFF_CHANNELS];

    uint32_t m_pinStateChangeTime[MAX_ON_OFF_CHANNELS];

    bool m_pinStates[MAX_ON_OFF_CHANNELS];
    ConfigPins *m_configPins;
    MessagePayload *m_payload;

    void (*m_handler)(uint8_t, bool) = NULL;
    

    void registerOnOffDetector(String name, uint8_t pin, bool invert, uint8_t idx)
    {
        m_channelNames[m_registeredChannels] = name;
        m_channels[m_registeredChannels] = pin;
        m_channelIndex[m_registeredChannels] = idx;
        m_invert[m_registeredChannels] = invert;

        pinMode(pin, INPUT);

        m_console->println("onoffdetector=configure; // " + name + " port: " + String(idx) + " pin: " + String(pin));

        m_registeredChannels++;
    }

    void configure(IOConfig *config)
    {
        if (config->GPIO1Config == GPIO_CONFIG_INPUT)
            registerOnOffDetector(config->GPIO1Name, m_configPins->Gpio1, m_configPins->InvertGpio1, 1);
        if (config->GPIO2Config == GPIO_CONFIG_INPUT)
            registerOnOffDetector(config->GPIO2Name, m_configPins->Gpio2, m_configPins->InvertGpio2, 2);
        if (config->GPIO3Config == GPIO_CONFIG_INPUT)
            registerOnOffDetector(config->GPIO3Name, m_configPins->Gpio3, m_configPins->InvertGpio3, 3);
        if (config->GPIO4Config == GPIO_CONFIG_INPUT)
            registerOnOffDetector(config->GPIO4Name, m_configPins->Gpio4, m_configPins->InvertGpio4, 4);
        if (config->GPIO5Config == GPIO_CONFIG_INPUT)
            registerOnOffDetector(config->GPIO5Name, m_configPins->Gpio5, m_configPins->InvertGpio5, 5);
        if (config->GPIO6Config == GPIO_CONFIG_INPUT)
            registerOnOffDetector(config->GPIO6Name, m_configPins->Gpio6, m_configPins->InvertGpio6, 6);
        if (config->GPIO7Config == GPIO_CONFIG_INPUT)
            registerOnOffDetector(config->GPIO7Name, m_configPins->Gpio7, m_configPins->InvertGpio6, 7);
        if (config->GPIO8Config == GPIO_CONFIG_INPUT)
            registerOnOffDetector(config->GPIO8Name, m_configPins->Gpio8, m_configPins->InvertGpio8, 8);
    }

public:
    OnOffDetector(Console *console, ConfigPins *configPins, MessagePayload *payload)
    {
        m_console = console;
        m_configPins = configPins;
        m_payload = payload;
    }

    void setup(IOConfig *config)
    {
        configure(config);
    }

    void loop()
    {
        for (int idx = 0; idx < m_registeredChannels; ++idx)
        {
            bool state = digitalRead(m_channels[idx]);
            if (m_invert[idx])
                state = !state;

            if(state != m_pinStates[idx])
            {
                if(millis() - m_pinStateChangeTime[idx] > 250)
                {
                    m_pinStateChangeTime[idx] = millis();
                    m_pinStates[idx] = state;
                    m_payload->ioValues->setValue((m_channelIndex[idx] - 1), m_pinStates[idx]);
                    if(m_handler != NULL)
                    {
                        m_handler(m_channelIndex[idx], m_pinStates[idx]);
                    }
                }
            }

        }
    }

    void setStateChangeHandler(void (*handler)(uint8_t, bool))
    {
        m_handler = handler;     
    }

    bool getPinState(int idx)
    {
        return m_pinStates[idx];
    }

    void debugPrint()
    {
        for (int idx = 0; idx < m_registeredChannels; ++idx)
        {
            m_console->printVerbose(m_channelNames[idx] + "=" + (m_pinStates[idx] ? "on" : "off"));
        }
    }
};

#endif