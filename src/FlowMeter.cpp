#include "FlowMeter.h"

#include <Arduino.h>

FlowMeter *flowMeter;

void isr1() {
    flowMeter->toggled(0);
}

void isr2() {
    flowMeter->toggled(1);
}

void isr3() {
    flowMeter->toggled(2);
}

void isr4() {
    flowMeter->toggled(3);
}

FlowMeter::FlowMeter(Console *console) {
    flowMeter = this;
    m_console = console;
}

void FlowMeter::toggled(int channel) {
    m_channelCounts[channel]++;
}

void FlowMeter::registerPin(uint8_t pin) {
    m_pins[m_channels] = pin;

    switch(m_channels++){
        case 0: attachInterrupt(pin, isr1, CHANGE); break;
        case 1: attachInterrupt(pin, isr2, CHANGE); break;
        case 2: attachInterrupt(pin, isr3, CHANGE); break;
        case 3: attachInterrupt(pin, isr4, CHANGE); break;
    }
}

void FlowMeter::setup() {
    for(int idx = 0; idx < NUMBER_FLOW_CHANNELS; ++idx) {
        m_flowRates[idx] = 0;
        m_channelCounts[idx] = 0;
        for(int slot = 0; slot < FLOW_BUFFER_SIZE; ++slot)
        {
            m_rateBuffer[idx][slot] = 0;
        }
    }

    for(int slot = 0; slot < FLOW_BUFFER_SIZE; ++slot){
        m_pins[slot] = -1;
    }
}

void FlowMeter::loop() {
    uint32_t millisDelta = millis() - m_lastMillis;
    if(millisDelta > SAMPLE_PERIOD) {
        m_lastMillis = millis();

        double factor
        for(int idx = 0; idx < NUMBER_FLOW_CHANNELS; ++idx)
        {
            m_rateBuffer[idx][m_slotIndex] = m_channelCounts[idx];
            m_channelCounts[idx] = 0;
        }

        m_slotIndex++;
        if(m_slotIndex == FLOW_BUFFER_SIZE)
        {
            m_firstPassCompleted = true;
            m_slotIndex = 0;
        }

        if(m_firstPassCompleted){
            for(int idx = 0; idx < m_channels; ++idx)
            {
                uint32_t countTotal = 0;
                for(int slot = 0; slot < FLOW_BUFFER_SIZE; ++slot)
                {
                    countTotal += m_rateBuffer[idx][slot];
                }

                m_flowRates[idx] = countTotal / 3; /* 6 500ms samples or 3 seconds, we want to get counts per second */
            }    
        }
    }
}

void FlowMeter::debugPrint() {
    if(m_firstPassCompleted){
        for(int idx = 0; idx < m_channels; ++idx)
        {
            m_console->printVerbose("COUNTS: " + String(idx) + "," + String(m_pins[idx]) + "," + String(m_flowRates[idx]) + " per second.");
        }
    }
}
