#ifndef MessagePayload_h
#define MessagePayload_h

#include <ArduinoJson.h>

class MessagePayload
{
public:
    String timeStamp;

    float current1;
    float current2;
    float current3;

    float voltage1;
    float voltage2;
    float voltage3;

    float temperature1;
    float temperature2;
    float temperature3;

    float humidity1;
    float humidity2;
    float humidity3;

    bool hasTemperature1;
    bool hasTemperature2;
    bool hasTemperature3;

    bool hasHumidity1;
    bool hasHumidity2;
    bool hasHumidity3;

    bool hasCurrent1;
    bool hasCurrent2;
    bool hasCurrent3;

    bool hasVoltage1;
    bool hasVoltage2;
    bool hasVoltage3;

    String getJSON();
};
#endif