#ifndef PowerSensor_h
#define PowerSensor_h

#include "MessagePayload.h"
#include "NuvIoTState.h"
#include "ADC.h"
#include "AbstractSensor.h"
#include "Console.h"
#include "IOConfig.h"
#include "ConfigPins.h"

#define NUMBER_CTS 3
#define MAX_SAMPLE_COUNT 200

class PowerSensor: public AbstractSensor {
    public:
        PowerSensor(ADC *adc, ConfigPins *configPins, Console *console, MessagePayload *payload, NuvIoTState *state);
        void loop();
        void debugPrint();

        void enableChannel(uint8_t channel, String name, float scaler);
        void disableChannel(uint8_t channel);
        float readAmps(uint8_t channel);
        float readWatts(uint8_t channel);        
        void setChannelVoltage(uint8_t channel, uint16_t voltage);

        /**
         * \brief sets sample period used for collecting AC samples.
         * 
         * Specify a period in milliseconds that will be used to capture A/C samples, the amount
         * should be enough to catch a number of cycles and should be period that will collect points
         * for complete samples.  For example for a 60Hz sample, if we specify a 333ms period, it will
         * collect 1/3 of a second or 1/3 the cycles, which for a 60Hz sample would be 20 complete samples.
         * 
         * \param samplePeriodMS number of milliseconds to collect AC samples (default is 333).
         * 
         **/
        void setSamplePeriodMS(uint16_t samplePeriodMS) {
            m_samplePeriodMS = samplePeriodMS;
        }
        void setup(IOConfig *ioConfig);
        void configure(IOConfig *ioConfig);

    private:
        bool m_isHealthy = false;
        uint16_t m_samplePeriodMS = 333;
        String m_lastError = "";

        String m_names[NUMBER_CTS];
        bool m_channelEnabled[NUMBER_CTS];
        float m_channelAmps[NUMBER_CTS];
        double m_ctRatioFactor[NUMBER_CTS];
        uint16_t m_voltage[NUMBER_CTS];

        float m_sampleBuffer[MAX_SAMPLE_COUNT];

        ConfigPins *m_configPins;
        ADC *m_adc;
        Console *m_console;
        NuvIoTState *m_state;
        MessagePayload *m_payload;    
};

#endif