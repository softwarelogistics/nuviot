#ifndef FLOWMETER_H
#define FLOWMETER_H

#include <Arduino.h>
#include "Console.h"

#define NUMBER_FLOW_CHANNELS 4
#define FLOW_BUFFER_SIZE 6
#define SAMPLE_PERIOD 500

class FlowMeter {
    private:
        Console *m_console;

        uint8_t m_channels;    
        uint32_t m_lastMillis;
        uint32_t m_channelCounts[NUMBER_FLOW_CHANNELS];
        uint8_t m_pins[NUMBER_FLOW_CHANNELS];
        double m_rateBuffer[NUMBER_FLOW_CHANNELS][FLOW_BUFFER_SIZE];
        volatile double m_flowRates[NUMBER_FLOW_CHANNELS];
        int m_slotIndex = 0;
        bool m_firstPassCompleted = false;

    public:
        FlowMeter(Console *channel);
        void toggled(int channel);
        void registerPin(uint8_t pin);
        void setup();
        void loop();
        void debugPrint();
};

#endif