#include "WaterSensor.h"

WaterSensor::WaterSensor(Adafruit_ADS1115 *ads, MessagePayload *payload, NuvIoTState *state)
{
    m_state = state;
    m_payload = payload;
    m_ads = ads;
}

void WaterSensor::setup()
{
    
}

void WaterSensor::loop()
{
    
}