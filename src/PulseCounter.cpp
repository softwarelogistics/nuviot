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


PulseCounter::PulseCounter(Console *console, ConfigPins *configPins, MessagePayload *payload) {
    __pulseCounter = this;
    m_console = console;
    m_configPins = configPins;
    m_payload = payload;
    for(uint8_t idx = 0; idx < 8; ++idx)
        m_portEnabled[idx] = 0;
}

void PulseCounter::toggled(int channel) {
    m_channelCounts[channel]++;
}

void PulseCounter::registerPin(uint8_t idx, String name, uint8_t pin) {
    m_names[idx] = name;
    m_portEnabled[idx] = 1;

    pinMode(pin, INPUT);

    switch(idx){
        case 0: attachInterrupt(pin, isr1, RISING); break;
        case 1: attachInterrupt(pin, isr2, RISING); break;
        case 2: attachInterrupt(pin, isr3, RISING); break;
        case 3: attachInterrupt(pin, isr4, RISING); break;
        case 4: attachInterrupt(pin, isr5, RISING); break;
        case 5: attachInterrupt(pin, isr6, RISING); break;
        case 6: attachInterrupt(pin, isr7, RISING); break;
        case 7: attachInterrupt(pin, isr8, RISING); break;
    }
}

void PulseCounter::applyValues(){
    for (int idx = 0; idx < NUMBER_PULSE_COUNTER_CHANNELS; ++idx){
        if (m_portEnabled[idx]){
            m_rawValues[idx] = m_pulsePerSecond[idx];// ((m_pulsePerSecond[idx] * m_calibration[idx]) - m_zero[idx]) * m_scalers[idx];
            m_payload->ioValues->setValue(idx, m_rawValues[idx]);
        }
        else
        {
            m_rawValues[idx] = -1;
        }
    }
}


void PulseCounter::setup(IOConfig *ioConfig) {
    setScaler(0, ioConfig->GPIO1Scaler);
    setScaler(1, ioConfig->GPIO2Scaler);
    setScaler(2, ioConfig->GPIO3Scaler);
    setScaler(3, ioConfig->GPIO4Scaler);
    setScaler(4, ioConfig->GPIO5Scaler);
    setScaler(5, ioConfig->GPIO6Scaler);
    setScaler(6, ioConfig->GPIO7Scaler);
    setScaler(7, ioConfig->GPIO8Scaler);

    setZero(0, ioConfig->GPIO1Zero);
    setZero(1, ioConfig->GPIO2Zero);
    setZero(2, ioConfig->GPIO3Zero);
    setZero(3, ioConfig->GPIO4Zero);
    setZero(4, ioConfig->GPIO5Zero);
    setZero(5, ioConfig->GPIO6Zero);
    setZero(6, ioConfig->GPIO7Zero);
    setZero(7, ioConfig->GPIO8Zero);

    setCalibration(0, ioConfig->GPIO1Calibration);
    setCalibration(1, ioConfig->GPIO2Calibration);
    setCalibration(2, ioConfig->GPIO3Calibration);
    setCalibration(3, ioConfig->GPIO4Calibration);
    setCalibration(4, ioConfig->GPIO5Calibration);
    setCalibration(5, ioConfig->GPIO6Calibration);
    setCalibration(6, ioConfig->GPIO7Calibration);
    setCalibration(7, ioConfig->GPIO8Calibration);

    for(int idx = 0; idx < NUMBER_PULSE_COUNTER_CHANNELS; ++idx) {
        m_pulsePerSecond[idx] = 0;
        m_channelCounts[idx] = 0;
        for(int slot = 0; slot < PULSE_COUNTER_AVERAGING_BUFFER_SIZE; ++slot)
        {
            m_pulseBuffer[idx][slot] = 0;
        }
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
            for(int channelIndex = 0; channelIndex < NUMBER_PULSE_COUNTER_CHANNELS; ++channelIndex)
            {
                uint32_t countTotal = 0;
                for(int slot = 0; slot < PULSE_COUNTER_AVERAGING_BUFFER_SIZE; ++slot)
                {
                    countTotal += m_pulseBuffer[channelIndex][slot];
                }

                m_pulsePerSecond[channelIndex] = countTotal / 3; /* 6 500ms samples or 3 seconds, we want to get counts per second */
            }    
        }

        applyValues();
    }
}

void PulseCounter::configure(IOConfig *config) {
    if(config->GPIO1Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(0, config->GPIO1Name, m_configPins->Gpio1 );
    if(config->GPIO2Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(1, config->GPIO2Name, m_configPins->Gpio2 );
    if(config->GPIO3Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(2, config->GPIO3Name, m_configPins->Gpio3 );
    if(config->GPIO4Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(3, config->GPIO4Name, m_configPins->Gpio4 );
    if(config->GPIO5Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(4, config->GPIO5Name, m_configPins->Gpio5 );
    if(config->GPIO6Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(5, config->GPIO6Name, m_configPins->Gpio6 );
    if(config->GPIO7Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(6, config->GPIO7Name, m_configPins->Gpio7 );
    if(config->GPIO8Config == GPIO_CONFIG_PULSE_COUNTER) registerPin(7, config->GPIO8Name, m_configPins->Gpio8 );
}

int PulseCounter::countsPerSecond(uint8_t channel){
    return m_pulsePerSecond[channel];
}

void PulseCounter::debugPrint() {
    if(m_firstPassCompleted){
        for(int idx = 0; idx < NUMBER_PULSE_COUNTER_CHANNELS; ++idx)
        {
            if(m_portEnabled[idx])
                m_console->printVerbose(m_names[idx] + "=" + String(m_pulsePerSecond[idx]));
        }
    }
}
