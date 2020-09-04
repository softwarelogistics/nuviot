#ifndef PULSECOUNTER_H
#define PULSECOUNTER_H

#include <Arduino.h>
#include "Console.h"
#include "IOConfig.h"
#include "ConfigPins.h"

#define NUMBER_PULSE_COUNTER_CHANNELS 8
#define PULSE_COUNTER_AVERAGING_BUFFER_SIZE 6
#define SAMPLE_PERIOD 500

class PulseCounter {
    private:
        Console *m_console;
        ConfigPins *m_configPins;
        uint8_t m_channels;    
        uint32_t m_lastMillis;
        uint32_t m_channelCounts[NUMBER_PULSE_COUNTER_CHANNELS];
        uint8_t m_pins[NUMBER_PULSE_COUNTER_CHANNELS];
        String m_names[NUMBER_PULSE_COUNTER_CHANNELS];
        volatile double m_pulsePerSecond[NUMBER_PULSE_COUNTER_CHANNELS];

        double m_pulseBuffer[NUMBER_PULSE_COUNTER_CHANNELS][PULSE_COUNTER_AVERAGING_BUFFER_SIZE];
        
        int m_slotIndex = 0;
        bool m_firstPassCompleted = false;

    public:
        PulseCounter(Console *channel, ConfigPins *configPins);
        void toggled(int channel);
        void registerPin(String name, uint8_t pin);
        void configure(IOConfig *config);
        void setup();
        void loop();
        void debugPrint();
        int countsPerSecond(uint8_t channel);
};

#endif