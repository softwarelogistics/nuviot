#include "WaterSensor.h"

WaterSensor::WaterSensor(Adafruit_ADS1115 *ads, MessagePayload *payload, NuvIoTState *state)
{
    m_state = state;
    m_payload = payload;
    m_ads = ads;
}

void WaterSensor::setup()
{
    strcpy(m_payload->panWater, "dry");
}

void WaterSensor::loop()
{
    int16_t wsvalue = m_ads->readADC_SingleEnded(0);
    if (wsvalue < 0)
    {
        m_payload->hasPanStatus = false;
    }
    else
    {
        m_payload->hasPanStatus = true;
        if (wsvalue < 5000)
            strcpy(m_payload->panWater, "dry");
        else if (wsvalue < 7000)
            strcpy(m_payload->panWater, "low");
        else if (wsvalue < 8500)
            strcpy(m_payload->panWater, "med");
        else
            strcpy(m_payload->panWater, "hgh");
    }
}