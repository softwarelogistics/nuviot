#include "PowerSensor.h"

PowerSensor::PowerSensor(ADC *adc, Logger *logger, MessagePayload *payload, NuvIoTState *state)
{
    m_adc = adc;
    m_logger = logger;
    m_payload = payload;
    m_state = state;
}

void PowerSensor::setup()
{
    m_channelEnabled[0] = false;
    m_channelEnabled[1] = false;
    m_channelEnabled[2] = false;

    m_voltage[0] = 120;
    m_voltage[1] = 120;
    m_voltage[2] = 120;

    m_ctRatioFactor[0] = 100.0f;
    m_ctRatioFactor[1] = 100.0f;
    m_ctRatioFactor[2] = 100.0f;

    
    m_adcChannels[0] = 2; /* ADCMOD1 - 2 */
    m_adcChannels[1] = 4; /* ADCMOD2 - 6 */    
    m_adcChannels[2] = 5; /* ADCMOD2 - 5 */
}

void PowerSensor::loop()
{
    for (int idx = 0; idx < 3; ++idx)
    {
        if (m_channelEnabled[idx])
        {
            float avereageTotal = 0;
            int iterations = 50;

            // Assuming 60 hz
            // sample once every 10ms or collect values over 30 samples
            for (int sampleIteration = 0; sampleIteration < iterations; ++sampleIteration)
            {
                float voltage = m_adc->getVoltage(m_adcChannels[idx]);
                avereageTotal += voltage;
                delay(10);
            }

            // over the course of exactly 30 (or very close to it) since we are measuring
            // an absolute voltage via a sine wave, the center should be right at zero
            // this will establishe our baseline (which for our circuit should be very close
            // to 2.5 volts).
            float offset = avereageTotal / iterations;
            Serial.println("Found average total: " + String(offset));

            float levelTotal = 0;

            for (int sampleIteration = 0; sampleIteration < iterations; ++sampleIteration)
            {
                float voltage = m_adc->getVoltage(m_adcChannels[idx]);
                /* start collecting the sum of the voltages */
                levelTotal += voltage > offset ? voltage - offset : -(voltage - offset);
                delay(10);
            }

            // we are using a 100A : 0.050MA CT, with the burden resistor it's
            // a factor of 100.  If the CT ratio changes, we may need to consider
            // adjusting this as well.  This should probably be pulled from a setting.
            m_channelAmps[idx] = (levelTotal / (float)iterations) * m_ctRatioFactor[idx];
        }
    }

    m_payload->hasCurrent1 = m_channelEnabled[0];
    m_payload->hasCurrent2 = m_channelEnabled[1];
    m_payload->hasCurrent3 = m_channelEnabled[2];
    m_payload->current1 = m_channelEnabled[0] ? m_channelAmps[0] : -1;
    m_payload->current2 = m_channelEnabled[1] ? m_channelAmps[1] : -1;
    m_payload->current3 = m_channelEnabled[2] ? m_channelAmps[2] : -1;
}

void PowerSensor::debugPrint() 
{
    for(int idx = 0; idx < 3; ++idx)
        if(m_channelEnabled[idx]) m_logger->logVerbose("AMPS" + String(idx) + "  :" + String(m_channelAmps[idx]));
}

void PowerSensor::setChannelVoltage(uint8_t channel, uint16_t voltage)
{
    m_voltage[channel] = voltage;
}

void PowerSensor::enableChannel(uint8_t channel, bool isEnabled)
{
    m_channelEnabled[channel] = isEnabled;
}

float PowerSensor::readAmps(uint8_t channel)
{
    m_channelAmps[channel] = channel;
}

/* P = I * E */
float PowerSensor::readWatts(uint8_t channel)
{
    return m_channelEnabled[channel] * m_voltage[channel];
}
