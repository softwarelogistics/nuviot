#ifndef TemperatureProbes_h
#define TemperatureProbes_h

#include <OneWire.h>
#include <DallasTemperature.h>
#include "MessagePayload.h"
#include "NuvIoTState.h"

class TemperatureProbes{
    public:
        TemperatureProbes(int pin, MessagePayload *payload, NuvIoTState *state);
        void setup();
        void findAddresses();
        void loop();

    private:
        DeviceAddress m_highThermometer;
        DeviceAddress m_lowThermometer;
        int m_addressCount = 0;
        byte m_addressBank [4][8];

        OneWire *m_oneWire;
        DallasTemperature *m_probes;

        int m_pin;
        MessagePayload* m_payload;

        NuvIoTState* m_state;
};
#endif