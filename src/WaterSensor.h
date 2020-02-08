#ifndef WaterSensor_h
#define WaterSensor_h

#include "Millian_Aire.h"
#include "MessagePayload.h"
#include "NuvIoTState.h"

class WaterSensor{
    public:
        WaterSensor(Adafruit_ADS1115 *ads, MessagePayload *payload, NuvIoTState *state);
        void setup();
        void loop();

    private:
        NuvIoTState *m_state;
        Adafruit_ADS1115 *m_ads; 
        MessagePayload* m_payload;
};
#endif