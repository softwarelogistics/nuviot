#ifndef PowerSensor_h
#define PowerSensor_h

#include "MessagePayload.h"
#include "NuvIoTState.h"
#include "ADC.h"
#include "AbstractSensor.h"
#include "Console.h"
#include "IOConfig.h"

#define NUMBER_CTS 3

class PowerSensor: public AbstractSensor {
    public:
        PowerSensor(ADC *adc, Console *console, MessagePayload *payload, NuvIoTState *state);
        void setup(IOConfig *ioConfig);
        void loop();
        void debugPrint();

        void enableChannel(uint8_t channel, String name, float scaler);
        void disableChannel(uint8_t channel);
        float readAmps(uint8_t channel);
        float readWatts(uint8_t channel);        
        void setChannelVoltage(uint8_t channel, uint16_t voltage);

        void configure(IOConfig *ioConfig);

    private:
        bool m_isHealthy = false;
        String m_lastError = "";

        String m_names[NUMBER_CTS];
        bool m_channelEnabled[NUMBER_CTS];
        float m_channelAmps[NUMBER_CTS];
        double m_ctRatioFactor[NUMBER_CTS];
        uint16_t m_voltage[NUMBER_CTS];

        ADC *m_adc;
        Console *m_console;
        NuvIoTState *m_state;
        MessagePayload *m_payload;    
};

#endif