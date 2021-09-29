#ifndef TemperatureProbes_h
#define TemperatureProbes_h

#include <OneWire.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include "MessagePayload.h"
#include "NuvIoTState.h"
#include "Console.h"
#include "AbstractSensor.h"
#include "ConfigPins.h"
#include "IOConfig.h"

#define PROBECOUNT 8

enum SensorConfigs {
    None,
    Dht11,
    Dht22,
    DS18B20    
};

#define SENSOR_CONFIG_DS18B20
#define SENSOR_CONFIG_DHT11
#define SENSOR_CONFIG_DHT22

class TemperatureProbes: public AbstractSensor{
    public:
        TemperatureProbes(Console *console, ConfigPins *configPins,  MessagePayload *payload);
        TemperatureProbes(Console *console, ConfigPins *configPins);

        void setup(IOConfig *ioConfig);
        void configure(IOConfig *ioConfig);
        void debugPrint();
        void loop();
        void loop(double values[]);

        float getTemperature(int idx);
        float getHumidity(int idx);

        void configureProbe(uint8_t idx, String name, uint8_t config);

    private:
        float m_temperatures[PROBECOUNT];
        float m_humidities[PROBECOUNT];
        
        bool m_initialized;
        IOConfig *m_ioConfig;
        ConfigPins *m_configPins;

        uint8_t resolvePinIndex(uint8_t idx);

        SensorConfigs m_sensorConfigurations[PROBECOUNT];

        OneWire *m_oneWires[PROBECOUNT];
        DallasTemperature *m_probes[PROBECOUNT];
        DHT *m_dhts[PROBECOUNT];
        String m_names[PROBECOUNT]; 
        uint8_t m_pins[PROBECOUNT];
        MessagePayload* m_payload = NULL;
        Console* m_console;

        void readTemperatures();
};
#endif