#include "TemperatureProbes.h"

TemperatureProbes::TemperatureProbes(Logger *logger, MessagePayload *payload, NuvIoTState *state)
{
    m_payload = payload;
    m_state = state;
    m_logger = logger;
}

void TemperatureProbes::setup()
{
    m_sensorConfigurations[0] = None;
    m_sensorConfigurations[1] = None;
    m_sensorConfigurations[2] = None;
}

#define MIN_VALUE -999

void TemperatureProbes::loop()
{
    for(int idx = 0; idx < 3; ++idx) {
        float temperature = MIN_VALUE;
        float humidity = MIN_VALUE;

        switch(m_sensorConfigurations[idx]) {
            case None:
                break;
            case DS18B20:
                temperature = m_probes[idx]->getTempFByIndex(0);
                m_probes[idx]->requestTemperatures();
                break;
            case Dht11:
            case Dht22:
                humidity = m_dhts[idx]->readHumidity();
                temperature = round((m_dhts[idx]->readTemperature() * 1.8f) + 32.0f);
                break;
        }

        if(temperature != MIN_VALUE)
            m_temperatures[idx] = temperature;

        if(humidity != MIN_VALUE)
            m_humidities[idx] = -20;

        switch(idx) {
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
                m_payload->humidity3 = humidity == MIN_VALUE  ? 0 : humidity;
                m_payload->hasHumidity3 = humidity != MIN_VALUE;
                break;
        }
    }
}

byte TemperatureProbes::resolvePinIndex(int idx) {
    if(idx == 0)  return 17;
    if (idx == 1) return 13;
    if (idx == 2) return 14;
    
    return -1;
}

void TemperatureProbes::configureProbe(int idx, SensorConfigs config)
{
    byte pin = resolvePinIndex(idx);
    if(pin == -1) {
        m_logger->logError("Invalid pin on configure probe.");
        return;
    }

    switch(config) {
        case None:  break;
        case Dht11: m_dhts[idx] = new DHT(pin, DHT21); break;
        case Dht22: m_dhts[idx] = new DHT(pin, DHT22); break;
        case DS18B20:
            m_oneWires[idx] = new OneWire(pin);
            m_probes[idx] = new DallasTemperature(m_oneWires[idx]);
            break;

    }

    m_sensorConfigurations[idx] = config;
}

float TemperatureProbes::getTemperature(int idx)
{
    switch(m_sensorConfigurations[idx]) {
        case DS18B20:
        case Dht11:
        case Dht22:
            return m_temperatures[idx];
    }

    return 0.0f;
}

float TemperatureProbes::getHumidity(int idx)
{
    switch(m_sensorConfigurations[idx]) {
        case Dht11:
        case Dht22:
            return m_humidities[idx];
            break;
    }

    return 0.0f;
}

