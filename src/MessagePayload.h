#ifndef MessagePayload_h
#define MessagePayload_h

#include <ArduinoJson.h>

class MessagePayload{
    public:
        String timeStamp;
        float ambientTemperature;
        float compressorCurrent;
        float blowerCurrent;
        float highPressureRaw;
        double highPressure;
        float lowPressureRaw;
        double lowPressure;
        float ambientHumidity;
        float highTemperature;
        float lowTemperature;
        char panWater[10];

        bool isWeatherOnline;
        bool areProbesOnline;
        bool hasCompressorCurrent;
        bool hasBlowerCurrent;
        bool hasLowPressure;
        bool hasHighPressure;
        bool hasPanStatus;

        String getJSON();
        void generateSampleData();
};
#endif