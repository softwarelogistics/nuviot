#ifndef PowerSensor_h
#define PowerSensor_h

#include "MessagePayload.h"
#include "NuvIoTState.h"
#include "ADC.h"
#include "Logger.h"
#include "AbstractSensor.h"

class PowerSensor: public AbstractSensor {
    public:
        PowerSensor(ADC *adc, Logger *logger, MessagePayload *payload, NuvIoTState *state);
        void setup();
        void loop();
        void debugPrint();

        void enableChannel(uint8_t channel, bool isEnabled);
        void disableChannel(uint8_t channel);
        float readAmps(uint8_t channel);
        float readWatts(uint8_t channel);        
        void setChannelVoltage(uint8_t channel, uint16_t voltage);

    private:
        uint8_t m_adcChannels[3];

        bool m_isHealthy = false;
        String m_lastError = "";

        bool m_channelEnabled[3];
        float m_channelAmps[3];
        double m_ctRatioFactor[3];
        uint16_t m_voltage[3];

        ADC *m_adc;
        Logger *m_logger;
        NuvIoTState *m_state;
        MessagePayload *m_payload;    
};

#endif