#ifndef TemperatureProbes_h
#define TemperatureProbes_h

#include <OneWire.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include "MessagePayload.h"
#include "NuvIoTState.h"
#include "Console.h"
#include "AbstractSensor.h"

#define PROBECOUNT 5

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
        TemperatureProbes(Console *console, MessagePayload *payload);
        TemperatureProbes(Console *console);

        void setup();
        void debugPrint();
        void loop();

        float getTemperature(int idx);
        float getHumidity(int idx);

        void configureProbe(int idx, SensorConfigs config);

    private:
        float m_temperatures[PROBECOUNT];
        float m_humidities[PROBECOUNT];
        bool m_initialized;

        byte resolvePinIndex(int idx);

        SensorConfigs m_sensorConfigurations[PROBECOUNT];

        OneWire *m_oneWires[PROBECOUNT];
        DallasTemperature *m_probes[PROBECOUNT];
        DHT *m_dhts[PROBECOUNT]; 
        
        MessagePayload* m_payload = NULL;
        Console* m_console;

        void readTemperatures();
};
#endif