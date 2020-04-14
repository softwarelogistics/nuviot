#ifndef TemperatureProbes_h
#define TemperatureProbes_h

#include <OneWire.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include "MessagePayload.h"
#include "NuvIoTState.h"
#include "Logger.h"
#include "AbstractSensor.h"

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
        TemperatureProbes(Logger *logger, MessagePayload *payload, NuvIoTState *state);

        void setup();
        void debugPrint();
        void loop();

        float getTemperature(int idx);
        float getHumidity(int idx);

        void configureProbe(int idx, SensorConfigs config);

    private:
        float m_temperatures[3];
        float m_humidities[3];
        bool m_initialized;

        byte resolvePinIndex(int idx);

        SensorConfigs m_sensorConfigurations[3];

        OneWire *m_oneWires[3];
        DallasTemperature *m_probes[3];
        DHT *m_dhts[3]; 
        
        MessagePayload* m_payload;
        Logger* m_logger;

        NuvIoTState* m_state;        

        void readTemperatures();
};
#endif