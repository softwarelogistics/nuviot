#include "PowerSensor.h"
#include "Millian_Aire.h"

PowerSensor::PowerSensor(MessagePayload *payload, NuvIoTState *state)
{
    m_payload = payload;
    m_state = state;
}

void PowerSensor::setup()
{
    m_compressorCal = m_state->getFlt(PARAM_PWR_CAL_COMPRESSOR);
    m_blowerCal = m_state->getFlt(PARAM_PWR_CAL_BLOWER);

    m_emonCompressor = new EnergyMonitor();
    m_emonCompressor->current(38, m_compressorCal); 

    m_emonBlower = new EnergyMonitor();
    m_emonBlower->current(39, m_blowerCal);

    Serial.println("Power calibration startup");
    Serial.println(String(m_state->getFlt(PARAM_PWR_CAL_BLOWER)) + " " + String());
}

void PowerSensor::loop()
{
    float calFactorBlower = m_state->getFlt(PARAM_PWR_CAL_BLOWER);
    float calFactorCompressor = m_state->getFlt(PARAM_PWR_CAL_COMPRESSOR);

    if(m_compressorCal != calFactorCompressor){
        m_emonCompressor->current(38, calFactorCompressor);
        m_compressorCal = calFactorCompressor;
    }
    
    if(m_blowerCal != calFactorBlower) {
        m_emonBlower->current(39, calFactorBlower);
        m_blowerCal = calFactorBlower;
    }
    
    m_payload->compressorCurrent = (int)(m_emonCompressor->calcIrms(1480) * 10) / 10.0f;
    m_payload->blowerCurrent = (int)(m_emonBlower->calcIrms(1480) * 10) / 10.0f;
    m_payload->hasBlowerCurrent = true;
    m_payload->hasCompressorCurrent = true;
}