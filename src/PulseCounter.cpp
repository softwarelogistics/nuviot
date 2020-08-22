#include "PulseCounter.h"

#include <Arduino.h>

PulseCounter *__pulseCounter;

void isr1() {
    __pulseCounter->toggled(0);
}

void isr2() {
    __pulseCounter->toggled(1);
}

void isr3() {
    __pulseCounter->toggled(2);
}

void isr4() {
    __pulseCounter->toggled(3);
}

PulseCounter::PulseCounter(Console *console) {
    __pulseCounter = this;
    m_console = console;
}

void PulseCounter::toggled(int channel) {
    m_channelCounts[channel]++;
}

void PulseCounter::registerPin(uint8_t pin) {
    m_pins[m_channels] = pin;

    pinMode(pin, INPUT);

    switch(m_channels++){
        case 0: attachInterrupt(pin, isr1, CHANGE); break;
        case 1: attachInterrupt(pin, isr2, CHANGE); break;
        case 2: attachInterrupt(pin, isr3, CHANGE); break;
        case 3: attachInterrupt(pin, isr4, CHANGE); break;
    }
}

void PulseCounter::setup() {
    for(int idx = 0; idx < NUMBER_FLOW_CHANNELS; ++idx) {
        m_pulsePerSecond[idx] = 0;
        m_channelCounts[idx] = 0;
        for(int slot = 0; slot < FLOW_BUFFER_SIZE; ++slot)
        {
            m_pulseBuffer[idx][slot] = 0;
        }
    }

    for(int slot = 0; slot < FLOW_BUFFER_SIZE; ++slot){
        m_pins[slot] = -1;
    }
}

void PulseCounter::loop() {
    uint32_t millisDelta = millis() - m_lastMillis;
    if(millisDelta > SAMPLE_PERIOD) {
        m_lastMillis = millis();

        // we always want the number of counts per 500 ms in each slot.
        double factor = (double)SAMPLE_PERIOD / (double)millisDelta;
        for(int idx = 0; idx < NUMBER_FLOW_CHANNELS; ++idx)
        {
            m_pulseBuffer[idx][m_slotIndex] = (int)(m_channelCounts[idx] * factor);
            m_channelCounts[idx] = 0;
        }

        m_slotIndex++;
        if(m_slotIndex == FLOW_BUFFER_SIZE)
        {
            m_firstPassCompleted = true;
            m_slotIndex = 0;
        }

        if(m_firstPassCompleted){
            for(int channelIndex = 0; channelIndex < m_channels; ++channelIndex)
            {
                uint32_t countTotal = 0;
                for(int slot = 0; slot < FLOW_BUFFER_SIZE; ++slot)
                {
                    countTotal += m_pulseBuffer[channelIndex][slot];
                }

                m_pulsePerSecond[channelIndex] = countTotal / 3; /* 6 500ms samples or 3 seconds, we want to get counts per second */
            }    
        }
    }
}

int PulseCounter::countsPerSecond(uint8_t channel){
    return m_pulsePerSecond[channel];
}

void PulseCounter::debugPrint() {
    if(m_firstPassCompleted){
        for(int idx = 0; idx < m_channels; ++idx)
        {
            m_console->printVerbose("COUNTS: " + String(idx) + "," + String(m_pins[idx]) + "," + String(m_pulsePerSecond[idx]) + " per second.");
        }
    }
}
