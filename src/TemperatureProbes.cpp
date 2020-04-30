#include "TemperatureProbes.h"

TemperatureProbes::TemperatureProbes(Console *console, MessagePayload *payload, NuvIoTState *state)
{
    m_payload = payload;
    m_state = state;
    m_console = console;
}

TemperatureProbes::TemperatureProbes(Console *console, NuvIoTState *state)
{
    m_payload = NULL;
    m_state = state;
    m_console = console;
}

void TemperatureProbes::setup()
{
    for (int idx = 0; idx < 3; ++idx)
    {
        m_sensorConfigurations[idx] = None;
        m_probes[idx] = NULL;
        m_oneWires[idx] = NULL;
        m_dhts[idx] = NULL;
    }
}

#define MIN_VALUE -9999

void TemperatureProbes::readTemperatures()
{
    for (int idx = 0; idx < 3; ++idx)
    {
        float temperature = MIN_VALUE;
        float humidity = MIN_VALUE;

        switch (m_sensorConfigurations[idx])
        {
        case None:
            break;
        case DS18B20:
        {
            bool success = false;
            int retryCount = 0;
         
            while (!success && retryCount++ < 10)
            {
                m_probes[idx]->requestTemperatures();
                temperature = m_probes[idx]->getTempFByIndex(0);
                if (temperature < -100 || temperature > 175)
                {
                    delay(500);
                    m_console->printError("ERR DS18B20 Attempt: " + String(retryCount));
                }
                else
                {
                    success = true;
                }
            }

            if (!success)
            {
                temperature = -999;
            }
        }

        break;
        case Dht11:
        case Dht22:
            humidity = m_dhts[idx]->readHumidity();
            temperature = 32.0f + round(m_dhts[idx]->readTemperature() * 18.0f) / 10.0f;
            break;
        }

        if (temperature != MIN_VALUE)
            m_temperatures[idx] = temperature;

        if (humidity != MIN_VALUE)
            m_humidities[idx] = humidity;

        if (m_payload != NULL)
        {
            switch (idx)
            {
            case 0:
                m_payload->temperature1 = temperature == MIN_VALUE ? 0 : temperature;
                m_payload->hasTemperature1 = temperature != MIN_VALUE;
                m_payload->humidity1 = humidity == MIN_VALUE ? 0 : humidity;
                m_payload->hasHumidity1 = humidity != MIN_VALUE;
                break;
            case 1:
                m_payload->temperature2 = temperature == MIN_VALUE ? 0 : temperature;
                m_payload->hasTemperature2 = temperature != MIN_VALUE;
                m_payload->humidity2 = humidity == MIN_VALUE ? 0 : humidity;
                m_payload->hasHumidity2 = humidity != MIN_VALUE;
                break;
            case 2:
                m_payload->temperature3 = temperature == MIN_VALUE ? 0 : temperature;
                m_payload->hasTemperature3 = temperature != MIN_VALUE;
                m_payload->humidity3 = humidity == MIN_VALUE ? 0 : humidity;
                m_payload->hasHumidity3 = humidity != MIN_VALUE;
                break;
            }
        }
    }
}

void TemperatureProbes::loop()
{
    if (m_initialized)
    {
        readTemperatures();
    }
    else
    {
        /* first time through read it twice, that way the next we call this, every time
           we read a temperature, we get the one requested in the last loop, since the first
           time through we haven't requested it before...do it this way.*/

        for (int idx = 0; idx < 2; ++idx)
        {
            readTemperatures();
            delay(500);
        }
    }

    m_initialized = true;
}

void TemperatureProbes::debugPrint()
{
    for (int idx = 0; idx < 3; ++idx)
    {
        if (m_sensorConfigurations[idx] != None)
        {
            m_console->printVerbose("TEMP" + String(idx) + "  :" + String(getTemperature(idx)));
        }

        if (m_sensorConfigurations[idx] == Dht11 ||
            m_sensorConfigurations[idx] == Dht22)
        {
            m_console->printVerbose("HUMD" + String(idx) + "  :" + String(getHumidity(idx)));
        }
    }
}

byte TemperatureProbes::resolvePinIndex(int idx)
{
    if (idx == 0)
        return 17;
    if (idx == 1)
        return 13;
    if (idx == 2)
        return 14;

    return -1;
}

void TemperatureProbes::configureProbe(int idx, SensorConfigs config)
{
    byte pin = resolvePinIndex(idx);
    if (pin == -1)
    {
        m_console->printVerbose("Invalid pin on configure probe.");
        return;
    }

    switch (config)
    {
    case None:
        if (m_dhts[idx] != NULL)
        {
            delete m_dhts[idx];
            m_dhts[idx] = NULL;
        }

        if (m_probes[idx] != NULL)
        {
            delete m_probes[idx];
            m_probes[idx] = NULL;
        }

        if (m_oneWires[idx] != NULL)
        {
            delete m_oneWires[idx];
            m_oneWires[idx] = NULL;
        }

        break;
    case Dht11:
        if (m_probes[idx] != NULL)
        {
            delete m_probes[idx];
            m_probes[idx] = NULL;
        }

        if (m_oneWires[idx] != NULL)
        {
            delete m_oneWires[idx];
            m_oneWires[idx] = NULL;
        }

        if (m_dhts[idx] == NULL)
        {
            m_dhts[idx] = new DHT(pin, DHT21);
        }
        break;
    case Dht22:
        if (m_probes[idx] != NULL)
        {
            delete m_probes[idx];
            m_probes[idx] = NULL;
        }

        if (m_oneWires[idx] != NULL)
        {
            delete m_oneWires[idx];
            m_oneWires[idx] = NULL;
        }

        if (m_dhts[idx] == NULL)
        {
            m_dhts[idx] = new DHT(pin, DHT22);
        }
        break;
    case DS18B20:
        if (m_dhts[idx] != NULL)
        {
            delete m_dhts[idx];
            m_dhts[idx] = NULL;
        }

        if (m_oneWires[idx] == NULL)
        {
            m_oneWires[idx] = new OneWire(pin);
        }

        if (m_probes[idx] == NULL)
        {
            m_probes[idx] = new DallasTemperature(m_oneWires[idx]);
        }
        break;
    }

    m_sensorConfigurations[idx] = config;
}

float TemperatureProbes::getTemperature(int idx)
{
    switch (m_sensorConfigurations[idx])
    {
    case DS18B20:
    case Dht11:
    case Dht22:
        return m_temperatures[idx];
    }

    return 0.0f;
}

float TemperatureProbes::getHumidity(int idx)
{
    switch (m_sensorConfigurations[idx])
    {
    case Dht11:
    case Dht22:
        return m_humidities[idx];
        break;
    }

    return 0.0f;
}
