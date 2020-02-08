#ifndef PressureSensor_h
#define PressureSensor_h

#include "Millian_Aire.h"
#include "MessagePayload.h"
#include "NuvIoTState.h"

class PressureSensor{
    public:
        PressureSensor(Adafruit_ADS1115 *ads, MessagePayload *payload, NuvIoTState *state);
        void setup();
        void loop();

    private:
        NuvIoTState *m_state;
        Adafruit_ADS1115 *m_ads; /* Use this for the 16-bit version */
        MessagePayload* m_payload;
};
#endif