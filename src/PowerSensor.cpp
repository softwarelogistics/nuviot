#include "PowerSensor.h"

PowerSensor::PowerSensor(ADC *adc, ConfigPins *configPins, Console *console, MessagePayload *payload, NuvIoTState *state)
{
    m_adc = adc;
    m_console = console;
    m_payload = payload;
    m_state = state;
    m_configPins = configPins;
}

void PowerSensor::setup(IOConfig *config)
{
    m_voltage[0] = 0;
    m_voltage[1] = 0;
    m_voltage[2] = 0;

    configure(config);
}

void PowerSensor::loop()
{
    uint8_t adcChannel = -1;

    for (int idx = 0; idx < 3; ++idx)
    {
        switch(idx){
            case 0: adcChannel = m_configPins->CTChannel1; break;
            case 1: adcChannel = m_configPins->CTChannel2; break;
            case 2: adcChannel = m_configPins->CTChannel3; break;            
        }

        if(adcChannel == -1) {
            return;
        }

        if (m_channelEnabled[idx])
        {
            float avereageTotal = 0;
            int iterations = 50;

            float min = 500;
            float max = -1;

            // Assuming 60 hz
            // sample once every 10ms or collect values over 30 samples
            for (int sampleIteration = 0; sampleIteration < iterations; ++sampleIteration)
            {
                float voltage = m_adc->getVoltage(adcChannel);
                avereageTotal += voltage;
                delay(10);
            }

            // over the course of exactly 30 (or very close to it) since we are measuring
            // an absolute voltage via a sine wave, the center should be right at zero
            // this will establishe our baseline (which for our circuit should be very close
            // to 2.5 volts).
            float offset = avereageTotal / iterations;

            float levelTotal = 0;

            for (int sampleIteration = 0; sampleIteration < iterations; ++sampleIteration)
            {
                float voltage = m_adc->getVoltage(adcChannel);
                if (voltage < min)
                    min = voltage;

                if (voltage > max)
                    max = voltage;

                /* start collecting the sum of the voltages */
                levelTotal += voltage > offset ? voltage - offset : -(voltage - offset);
                delay(10);
            }

            float avgLevel = (levelTotal / (float)iterations);

            // i = E / R, burden resistor = 33.0
            // TODO: may need to adjust this with different burden resistor settings.            
            //double current = avgLevel / 33.0f;

            // we are using a 100A : 0.050MA CT, with the burden resistor it's
            // a factor of 100.  If the CT ratio changes, we may need to consider
            // adjusting this as well.  This should probably be pulled from a setting.
            m_channelAmps[idx] = avgLevel * m_ctRatioFactor[idx];
        }
    }

    m_payload->hasCurrent1 = m_channelEnabled[0];
    m_payload->hasCurrent2 = m_channelEnabled[1];
    m_payload->hasCurrent3 = m_channelEnabled[2];
    m_payload->current1 = m_channelEnabled[0] ? m_channelAmps[0] : -1;
    m_payload->current2 = m_channelEnabled[1] ? m_channelAmps[1] : -1;
    m_payload->current3 = m_channelEnabled[2] ? m_channelAmps[2] : -1;
}

void PowerSensor::configure(IOConfig *config) {
    if(config->ADC1Config == ADC_CONFIG_CT) enableChannel(0, config->ADC1Name, config->ADC1Scaler);
    if(config->ADC2Config == ADC_CONFIG_CT) enableChannel(1, config->ADC1Name, config->ADC2Scaler);
    if(config->ADC3Config == ADC_CONFIG_CT) enableChannel(2, config->ADC1Name, config->ADC3Scaler);
}

void PowerSensor::enableChannel(uint8_t channel, String name, float scaler)
{
    m_names[channel] = name;
    m_channelEnabled[channel] = true;
    
    m_adc->enableADC(name, channel, true);
    m_ctRatioFactor[channel] = (scaler);
}

void PowerSensor::debugPrint()
{
    for (int idx = 0; idx < 3; ++idx) {
        if (m_channelEnabled[idx])
        {
            m_console->printVerbose("AMPS" + String(idx) + "  :" + String(m_channelAmps[idx]));
        }
    }
}

void PowerSensor::setChannelVoltage(uint8_t channel, uint16_t voltage)
{
    m_voltage[channel] = voltage;
}

float PowerSensor::readAmps(uint8_t channel)
{
    m_channelAmps[channel] = channel;
    return m_channelAmps[channel];
}

/* P = I * E */
float PowerSensor::readWatts(uint8_t channel)
{
    return m_channelEnabled[channel] * m_voltage[channel];
}
