#include "PressureSensor.h"
#include <Wire.h>

PressureSensor::PressureSensor(Adafruit_ADS1115 *ads, MessagePayload *payload, NuvIoTState *state)
{
    m_payload = payload;
    m_ads = ads;
    m_state = state;
}

void PressureSensor::setup()
{
    Serial.println(String(m_state->getInt(PARAM_PWR_CAL_PRESSURE1)) + " " + String(m_state->getInt(PARAM_PWR_CAL_PRESSURE2)));
}

void PressureSensor::loop()
{
   
}