#include "PowerSensor.h"

PowerSensor::PowerSensor(ADC *adc, ConfigPins *configPins, Console *console, Display *display, MessagePayload *payload, NuvIoTState *state)
{
    m_adc = adc;
    m_console = console;
    m_payload = payload;
    m_state = state;
    m_configPins = configPins;
    m_display = display;
}

void PowerSensor::setup(IOConfig *config)
{
    for (int idx = 0; idx < 3; ++idx)
    {
        m_channelEnabled[idx] = false;
        m_voltage[idx] = 0;
        m_channelAmps[idx] = 0;
    }

    configure(config);
}

void PowerSensor::loop()
{
    uint8_t adcChannel = -1;

    for (int idx = 0; idx < 3; ++idx)
    {
        switch (idx)
        {
        case 0:
            adcChannel = m_configPins->CTChannel1;
            break;
        case 1:
            adcChannel = m_configPins->CTChannel2;
            break;
        case 2:
            adcChannel = m_configPins->CTChannel3;
            break;
        }

        if (adcChannel == -1)
        {
            m_console->printWarning("ADC CHANNEL: " + String(idx) + " not configured, aborting power sampling.");
            return;
        }

        if (m_channelEnabled[idx])
        {
            int iterations = 33;

            int samplePeriodMS = 333; // 333 MS or 1/3 a second, or 20 cycles, may have to adjust this for 50Hz or otehrs.

            long startAvg = millis();
            iterations = 0;

            while (millis() - startAvg < samplePeriodMS)
            {
                m_sampleBuffer[iterations] = m_adc->getVoltage(adcChannel);
                iterations++;
            }

            float baselineTotal = 0;
            for (int idx = 0; idx < iterations; ++idx)
            {
                baselineTotal += m_sampleBuffer[idx];
            }

            float baseline = baselineTotal / (float)iterations;

            float absTotal = 0;
            for (int idx = 0; idx < iterations; ++idx)
            {
                float voltage = m_sampleBuffer[idx];
                absTotal += (voltage > baseline ? voltage - baseline : -(voltage - baseline));
            }

            float avgLevel = absTotal / (float)iterations;
            m_channelAmps[idx] = avgLevel * m_ctRatioFactor[idx];
            m_console->printVerbose("BASELINE: " + String(baseline) + ",  " + String(absTotal) + ",  "  + String(avgLevel) + ",  " + String(iterations) + ",  " + String(m_channelAmps[idx]));
        }
    }

    m_payload->hasCurrent1 = m_channelEnabled[0];
    m_payload->hasCurrent2 = m_channelEnabled[1];
    m_payload->hasCurrent3 = m_channelEnabled[2];
    m_payload->current1 = m_channelEnabled[0] ? m_channelAmps[0] : -1;
    m_payload->current2 = m_channelEnabled[1] ? m_channelAmps[1] : -1;
    m_payload->current3 = m_channelEnabled[2] ? m_channelAmps[2] : -1;
}

void PowerSensor::configure(IOConfig *config)
{
    if (config->ADC1Config == ADC_CONFIG_CT)
        enableChannel(0, config->ADC1Name, config->ADC1Scaler);
    if (config->ADC2Config == ADC_CONFIG_CT)
        enableChannel(1, config->ADC2Name, config->ADC2Scaler);
    if (config->ADC3Config == ADC_CONFIG_CT)
        enableChannel(2, config->ADC3Name, config->ADC3Scaler);

    if(!m_adc->isBank1Enabled())
    {
        m_console->println("adcbank1=disabled;");
    }
    else if (!m_adc->isBankOneOnline())
    {
        bool toggle = false;
        while (1)
        {
            m_display->clearBuffer();
            m_display->setTextSize(2);
            toggle = !toggle;
            if (toggle)
                m_display->drawStr("ERROR", "ADC BANK1", "OFFLINE", "!!!!");
            else
                m_display->drawStr("ERROR", "ADC BANK1", "OFFLINE");
            m_display->sendBuffer();
            m_console->printError("adcbank1=notready;");
            m_state->loop();
            delay(1000);
        };
    }
    else
    {
        m_console->println("adcbank1=online;");
    }

    if(!m_adc->isBank2Enabled())
    {
        m_console->println("adcbank2=disabled;");
    }
    else if (!m_adc->isBankOneOnline())
    if (m_adc->isBank2Enabled() && !m_adc->isBankTwoOnline())
    {
        m_console->printError("adcbank2=notready;");
                bool toggle = false;
        while (1)
        {
            m_display->clearBuffer();
            m_display->setTextSize(2);
            toggle = !toggle;
            if (toggle)
                m_display->drawStr("ERROR", "ADC BANK2", "OFFLINE", "!!!!");
            else
                m_display->drawStr("ERROR", "ADC BANK2", "OFFLINE");
            m_display->sendBuffer();

            m_console->printError("adcbank2=notready;");
            m_state->loop();
            delay(1000);
        };
    }
    else
    {
        m_console->println("adcbank2=online;");
    }

    m_adc->setConvesionDelay(10);
}

void PowerSensor::enableChannel(uint8_t channel, String name, float scaler)
{
    m_names[channel] = name;
    m_channelEnabled[channel] = true;
    m_ctRatioFactor[channel] = (scaler);

    uint8_t adcChannel = -1;

    switch(channel) {
        case 0:  adcChannel = m_configPins->CTChannel1; break;
        case 1:  adcChannel = m_configPins->CTChannel2; break;
        case 2:  adcChannel = m_configPins->CTChannel3; break;
    }

    if(adcChannel < 0)
    {
        m_console->printError("invalidchannel=ct" + String(channel) + "; // Invalid mapping for a ct");
    }
    else
    {
        m_console->println("ct" + String(channel) + ",adcchannel=" + String(adcChannel) + ",name+ " + name + "=enabled;scaler=" + String(scaler) + ";");
        m_adc->enableADCAsCT(name, adcChannel, true);
    }
}

void PowerSensor::debugPrint()
{
    for (int idx = 0; idx < 3; ++idx)
    {
        if (m_channelEnabled[idx])
        {
            m_console->printVerbose(m_names[idx] + "=" + String(m_channelAmps[idx]) + ";");
        }
    }
}

void PowerSensor::setChannelVoltage(uint8_t channel, uint16_t voltage)
{
    m_voltage[channel] = voltage;
}

float PowerSensor::readAmps(uint8_t channel)
{
    return m_channelAmps[channel];
}

/* P = I * E */
float PowerSensor::readWatts(uint8_t channel)
{
    return m_channelEnabled[channel] * m_voltage[channel];
}
