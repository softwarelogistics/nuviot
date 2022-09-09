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

void isr5() {
    __pulseCounter->toggled(4);
}

void isr6() {
    __pulseCounter->toggled(5);
}

void isr7() {
    __pulseCounter->toggled(6);
}

void isr8() {
    __pulseCounter->toggled(7);
}


PulseCounter::PulseCounter(Console *console, ConfigPins *configPins) {
    __pulseCounter = this;
    m_console = console;
    m_configPins = configPins;
    m_channels = 0;
}

void PulseCounter::toggled(int channel) {
    m_channelCounts[channel]++;
}

void PulseCounter::registerPin(String name, uint8_t pin) {
    m_pins[m_channels] = pin;
    m_names[m_channels] = name;

    pinMode(pin, INPUT);

    switch(m_channels++){
        case 0: attachInterrupt(pin, isr1, CHANGE); break;
        case 1: attachInterrupt(pin, isr2, CHANGE); break;
        case 2: attachInterrupt(pin, isr3, CHANGE); break;
        case 3: attachInterrupt(pin, isr4, CHANGE); break;
        case 4: attachInterrupt(pin, isr5, CHANGE); break;
        case 5: attachInterrupt(pin, isr6, CHANGE); break;
        case 6: attachInterrupt(pin, isr7, CHANGE); break;
        case 7: attachInterrupt(pin, isr8, CHANGE); break;
    }
}

void PulseCounter::setup(IOConfig *ioConfig) {
    for(int idx = 0; idx < NUMBER_PULSE_COUNTER_CHANNELS; ++idx) {
        m_pulsePerSecond[idx] = 0;
        m_channelCounts[idx] = 0;
        for(int slot = 0; slot < PULSE_COUNTER_AVERAGING_BUFFER_SIZE; ++slot)
        {
            m_pulseBuffer[idx][slot] = 0;
        }
    }

    for(int slot = 0; slot < PULSE_COUNTER_AVERAGING_BUFFER_SIZE; ++slot){
        m_pins[slot] = -1;
    }

    configure(ioConfig);
}

void PulseCounter::loop() {
    uint32_t millisDelta = millis() - m_lastMillis;
    if(millisDelta > SAMPLE_PERIOD) {
        m_lastMillis = millis();

        // we always want the number of counts per 500 ms in each slot.
        double factor = (double)SAMPLE_PERIOD / (double)millisDelta;
        for(int idx = 0; idx < NUMBER_PULSE_COUNTER_CHANNELS; ++idx)
        {
            m_pulseBuffer[idx][m_slotIndex] = (int)(m_channelCounts[idx] * factor);
            m_channelCounts[idx] = 0;
        }

        m_slotIndex++;
        if(m_slotIndex == PULSE_COUNTER_AVERAGING_BUFFER_SIZE)
        {
            m_firstPassCompleted = true;
            m_slotIndex = 0;
        }

        if(m_firstPassCompleted){
            for(int channelIndex = 0; channelIndex < m_channels; ++channelIndex)
            {
                uint32_t countTotal = 0;
                for(int slot = 0; slot < PULSE_COUNTER_AVERAGING_BUFFER_SIZE; ++slot)
                {
                    countTotal += m_pulseBuffer[channelIndex][slot];
                }

                m_pulsePerSecond[channelIndex] = countTotal / 3; /* 6 500ms samples or 3 seconds, we want to get counts per second */
            }    
        }
    }
}

void PulseCounter::configure(IOConfig *config) {
    if(config->GPIO1Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(config->GPIO1Name, m_configPins->Gpio1 );
    if(config->GPIO2Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(config->GPIO2Name, m_configPins->Gpio2 );
    if(config->GPIO3Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(config->GPIO3Name, m_configPins->Gpio3 );
    if(config->GPIO4Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(config->GPIO4Name, m_configPins->Gpio4 );
    if(config->GPIO5Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(config->GPIO5Name, m_configPins->Gpio5 );
    if(config->GPIO6Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(config->GPIO6Name, m_configPins->Gpio6 );
    if(config->GPIO7Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(config->GPIO7Name, m_configPins->Gpio7 );
    if(config->GPIO8Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(config->GPIO8Name, m_configPins->Gpio8 );
}

int PulseCounter::countsPerSecond(uint8_t channel){
    return m_pulsePerSecond[channel];
}

void PulseCounter::debugPrint() {
    if(m_firstPassCompleted){
        for(int idx = 0; idx < m_channels; ++idx)
        {
            m_console->printVerbose(m_names[idx] + "=" + String(m_pulsePerSecond[idx]));
        }
    }
}
