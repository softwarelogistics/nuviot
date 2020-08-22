#ifndef ONOFF_DETECTOR
#define ONOFF_DETECTOR

#include <Arduino.h>
#include "Console.h"

#define MAX_ON_OFF_CHANNELS 6

class OnOffDetector {
    
    private: 
        Console *m_console;
        uint8_t m_registeredChannels = 0;
        String m_channelNames[MAX_ON_OFF_CHANNELS];
        uint8_t m_channels[MAX_ON_OFF_CHANNELS];
        bool m_pinStates[MAX_ON_OFF_CHANNELS];

    public:
        OnOffDetector(Console *console) {
            m_console = console;
        }

        void setup() { }

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
};

#endif