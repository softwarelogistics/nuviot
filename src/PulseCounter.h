#ifndef PULSECOUNTER_H
#define PULSECOUNTER_H

#include <Arduino.h>
#include "Console.h"
#include "IOConfig.h"
#include "ConfigPins.h"
#include "AbstractSensor.h"
#include "MessagePayload.h"
#include <nvs.h>

#define NUMBER_PULSE_COUNTER_CHANNELS 8
#define PULSE_COUNTER_AVERAGING_BUFFER_SIZE 6
#define SAMPLE_PERIOD 500

class PulseCounter : public AbstractSensor {
    private:
        Console *m_console;
        ConfigPins *m_configPins;
        uint32_t m_lastMillis;
        MessagePayload *m_payload;
        nvs_handle m_nvsHandle;
        esp_timer_handle_t m_clearCountsTimerHandle;

        float m_calibration[NUMBER_PULSE_COUNTER_CHANNELS];
        float m_scalers[NUMBER_PULSE_COUNTER_CHANNELS];
        float m_zero[NUMBER_PULSE_COUNTER_CHANNELS];
        float m_frequencies[NUMBER_PULSE_COUNTER_CHANNELS];
        uint64_t m_channelCounts[NUMBER_PULSE_COUNTER_CHANNELS];

        float m_rawValues[NUMBER_PULSE_COUNTER_CHANNELS];
        
        uint8_t m_portEnabled[NUMBER_PULSE_COUNTER_CHANNELS];
        String m_names[NUMBER_PULSE_COUNTER_CHANNELS];

        void applyValues();

        double m_pulseBuffer[NUMBER_PULSE_COUNTER_CHANNELS][PULSE_COUNTER_AVERAGING_BUFFER_SIZE];
        
        int m_slotIndex = 0;

        void setScaler(uint8_t channel, float scaler)
        {
            m_scalers[channel] = scaler;
        }

        void setZero(uint8_t channel, float zero)
        {
            m_zero[channel] = zero;
        }

        void setCalibration(uint8_t channel, float calibration)
        {
            m_calibration[channel] = calibration;
        }

    public:
        PulseCounter(Console *channel, ConfigPins *configPins, MessagePayload *payload);
        void registerPin(uint8_t idx, String name, uint8_t pin);
        void configure(IOConfig *config);
        void setup(IOConfig *config);
        void loop();
        void debugPrint();
        int countsPerSecond(uint8_t channel);
};

#endif