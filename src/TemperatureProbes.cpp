#include "TemperatureProbes.h"

#define SL_BOARD_TYPE

TemperatureProbes::TemperatureProbes(Console *console, ConfigPins *configPins, MessagePayload *payload){
    m_payload = payload;
    m_console = console;
    m_configPins = configPins;
}

TemperatureProbes::TemperatureProbes(Console *console, ConfigPins *configPins){
    m_payload = NULL;
    m_console = console;
    m_configPins = configPins;
}

void TemperatureProbes::setup(IOConfig *ioConfig){
    for (int idx = 0; idx < PROBECOUNT; ++idx)
    {
        m_sensorConfigurations[idx] = None;
        m_probes[idx] = NULL;
        m_oneWires[idx] = NULL;
        m_dhts[idx] = NULL;
    }

    configure(ioConfig);
}

void TemperatureProbes::configure(IOConfig *ioConfig){
    this->configureProbe(0, ioConfig->GPIO1Name, ioConfig->GPIO1Config);
    this->configureProbe(1, ioConfig->GPIO2Name, ioConfig->GPIO2Config);
    this->configureProbe(2, ioConfig->GPIO3Name, ioConfig->GPIO3Config);
    this->configureProbe(3, ioConfig->GPIO4Name, ioConfig->GPIO4Config);
    this->configureProbe(4, ioConfig->GPIO5Name, ioConfig->GPIO5Config);
    this->configureProbe(5, ioConfig->GPIO6Name, ioConfig->GPIO6Config);
    this->configureProbe(6, ioConfig->GPIO7Name, ioConfig->GPIO7Config);
    this->configureProbe(7, ioConfig->GPIO8Name, ioConfig->GPIO8Config);
}

#define MIN_VALUE -9999

void TemperatureProbes::readTemperatures()
{
    for (int idx = 0; idx < PROBECOUNT; ++idx)
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
         
            while (!success && retryCount++ < 5)
            {
                m_probes[idx]->requestTemperatures();
                delay(100);
                temperature = m_probes[idx]->getTempFByIndex(0);
                if (temperature < -100 || temperature > 175)
                {
                    delay(500);
                    m_console->printError("ERR DS18B20- " + String(idx) + " " + String(m_pins[idx]) + " Attempt: " + String(retryCount));
                }
                else
                {
                    m_payload->ioValues->setValue(idx + 8, temperature);
                    success = true;
                }
            }

            if (success){   
                clearLastError();
            }
            else {
                temperature = NAN;
                setIsErrorState(true);
            }
        }

        break;
        case Dht11:
        case Dht22:
            bool success = false;
            int retryCount = 0;
            while(!success && retryCount++ < 2){
                if(m_dhts[idx]->read(true)) {
                    humidity = m_dhts[idx]->readHumidity();
                    temperature = 32.0f + round(m_dhts[idx]->readTemperature() * 18.0f) / 10.0f;
                    if(isnan(humidity) || isnan(temperature)){                     
                        m_console->printError("ERR DHT- " + String(idx) + " " + String(m_pins[idx]) + " Attempt: " + String(retryCount));
                    }
                    else {
                        success = true;
                        m_payload->ioValues->setValue(idx + 8, temperature);                    
                    }
                }
                else 
                   delay(2000);
            }

            if (success){
                clearLastError();
            }
            else {
                temperature = NAN;
                humidity = NAN;
                setIsErrorState(true);
            }

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
            case 3:
                m_payload->temperature4 = temperature == MIN_VALUE ? 0 : temperature;
                m_payload->hasTemperature4 = temperature != MIN_VALUE;
                m_payload->humidity4 = humidity == MIN_VALUE ? 0 : humidity;
                m_payload->hasHumidity4 = humidity != MIN_VALUE;
                break;

            case 4:
                m_payload->temperature5 = temperature == MIN_VALUE ? 0 : temperature;
                m_payload->hasTemperature5 = temperature != MIN_VALUE;
                m_payload->humidity5 = humidity == MIN_VALUE ? 0 : humidity;
                m_payload->hasHumidity5 = humidity != MIN_VALUE;
                break;


            }
        }
    }
}

void TemperatureProbes::loop(double values[])
{
    for(int idx = 0; idx < 8; ++idx){
        if(m_sensorConfigurations[idx] != None){
            m_payload->ioValues->setValue(idx + 8, values[idx]);
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
    for (int idx = 0; idx < PROBECOUNT; ++idx)
    {
        if (m_sensorConfigurations[idx] == DS18B20)
        {
            m_console->printVerbose(m_names[idx] + "=" + String(getTemperature(idx)));
        }
        else if (m_sensorConfigurations[idx] == Dht11 ||
            m_sensorConfigurations[idx] == Dht22)
        {
            m_console->printVerbose(m_names[idx] + "_t=" + String(getTemperature(idx)));
            m_console->printVerbose(m_names[idx] + "_h=" + String(getHumidity(idx)));
        }
    }
}

uint8_t TemperatureProbes::resolvePinIndex(uint8_t idx)
{
    if (idx == 0)
        return m_configPins->Gpio1;
    if (idx == 1)
        return m_configPins->Gpio2;
    if (idx == 2)
        return m_configPins->Gpio3;
    if (idx == 3)
        return m_configPins->Gpio4;
    if (idx == 4)
        return m_configPins->Gpio5;
    if (idx == 5)
        return m_configPins->Gpio6;
    if (idx == 6)
        return m_configPins->Gpio7;
    if (idx == 7)
        return m_configPins->Gpio8;

    return -1;
}

void TemperatureProbes::configureProbe(uint8_t idx, String name, uint8_t setting)
{
    SensorConfigs config = None;
    if(setting == GPIO_CONFIG_DBS18) config =  DS18B20;
    if(setting == GPIO_CONFIG_DHT11) config =  Dht11;
    if(setting == GPIO_CONFIG_DHT22) config =  Dht22;

    uint8_t pin = resolvePinIndex(idx);
    if (pin == -1)
    {
        m_console->printError("Invalid pin on configure probe.");
        return;
    }

    if(config != None) {
        m_names[idx] = name;
        m_pins[idx] = resolvePinIndex(idx); 
    }
    else {
        m_names[idx] = "n/a";
    }

    switch (config)
    {
    case None:
        m_console->println("Probe pin: " + String(pin) + " not configured as probe.");
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
        m_console->println("Probe pin: " + String(pin) + " configured as DHT11.");
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
            m_dhts[idx] = new NuvIoT_DHT(pin, DHT11, 6, m_console);
        }
        break;
    case Dht22:
        m_console->println("Probe pin: " + String(pin) + " configured as DHT22.");
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
            m_dhts[idx] = new NuvIoT_DHT(pin, DHT22, 6, m_console);
        }
        break;
    case DS18B20:
        m_console->println("Probe pin: " + String(pin) + " configured as DS1820B.");
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
    default:
        return 0.0;        
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
    default:
        return 0.0;
    }

    return 0.0f;
}
