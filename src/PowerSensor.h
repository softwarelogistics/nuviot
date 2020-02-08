#ifndef PowerSensor_h
#define PowerSensor_h

#include "MessagePayload.h"
#include <EmonLib.h>
#include "NuvIoTState.h"

class PowerSensor {
    public:
        PowerSensor(MessagePayload *payload, NuvIoTState *state);
        void setup();
        void loop();

    private:
        NuvIoTState *m_state;
        MessagePayload *m_payload;
        EnergyMonitor *m_emonCompressor;
        EnergyMonitor *m_emonBlower; 

        float m_blowerCal;
        float m_compressorCal;
};

#endif