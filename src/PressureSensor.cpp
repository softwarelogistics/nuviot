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
    Wire.beginTransmission(ADS1015_ADDRESS);
    uint8_t error = Wire.endTransmission();

    if (error != 0)
    {
        Serial.println("Pressure sensor is offline.");
        m_payload->hasLowPressure = false;
        m_payload->hasHighPressure = false;
    }
    else
    {
        float cal1 = m_state->getFlt(PARAM_PWR_CAL_PRESSURE1);
        float cal2 = m_state->getFlt(PARAM_PWR_CAL_PRESSURE2);

        cal1 = max(0.0f, cal1);
        cal1 = min(100.0f, cal1);

        cal2 = max(0.0f, cal2);
        cal2 = min(100.0f, cal2);

        int raw1 = m_ads->readADC_SingleEnded(3);

        if (raw1 > 0)
        {
            m_payload->highPressureRaw = ((float)raw1 / 27156.0f) * 5.0f;

            Serial.println(m_payload->highPressureRaw * 100.0f);

            m_payload->highPressure = (int)(m_payload->highPressureRaw * 100.0f) - 50;
            m_payload->hasHighPressure = true;
        }
        else
        {
            m_payload->hasHighPressure = false;
            m_payload->highPressureRaw = 0;
            m_payload->highPressure = 0;
        }

        int raw2 = m_ads->readADC_SingleEnded(2);
        if (raw2 > 0)
        {
            m_payload->lowPressureRaw =  ((float)raw2 / 27156.0f) * 5.0f;
            m_payload->lowPressure =  (int)(m_payload->lowPressureRaw * 100.0f) - 50;
            m_payload->hasLowPressure = true;
        }
        else
        {
            m_payload->hasLowPressure = false;
            m_payload->lowPressureRaw = 0;
            m_payload->lowPressure = 0;
        }
    }
}