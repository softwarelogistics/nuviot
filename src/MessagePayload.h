#ifndef MessagePayload_h
#define MessagePayload_h

#include <ArduinoJson.h>

class MessagePayload
{
public:
    MessagePayload()
    {
        status = "ok";
        lastError = "none";

        hasCurrent1 = false;
        hasCurrent2 = false;
        hasCurrent3 = false;

        hasTemperature1 = false;
        hasTemperature2 = false;
        hasTemperature3 = false;
        hasTemperature4 = false;
        hasTemperature5 = false;
        
        hasHumidity1 = false;
        hasHumidity2 = false;
        hasHumidity3 = false;
        hasHumidity4 = false;
        hasHumidity5 = false;

        hasVoltage1 = false;
        hasVoltage2 = false;
        hasVoltage3 = false;
        hasVoltage4 = false;
        hasVoltage5 = false;
        hasVoltage6 = false;
        hasVoltage7 = false;
        hasVoltage8 = false;
    }

    String timeStamp;

    String status;
    String lastError;

    float current1;
    float current2;
    float current3;

    float voltage1;
    float voltage2;
    float voltage3;
    float voltage4;
    float voltage5;
    float voltage6;
    float voltage7;
    float voltage8;

    float temperature1;
    float temperature2;
    float temperature3;
    float temperature4;
    float temperature5;

    float humidity1;
    float humidity2;
    float humidity3;
    float humidity4;
    float humidity5;

    bool hasTemperature1;
    bool hasTemperature2;
    bool hasTemperature3;
    bool hasTemperature4;
    bool hasTemperature5;

    bool hasHumidity1;
    bool hasHumidity2;
    bool hasHumidity3;
    bool hasHumidity4;
    bool hasHumidity5;

    bool hasCurrent1;
    bool hasCurrent2;
    bool hasCurrent3;

    bool hasVoltage1;
    bool hasVoltage2;
    bool hasVoltage3;
    bool hasVoltage4;
    bool hasVoltage5;
    bool hasVoltage6;
    bool hasVoltage7;
    bool hasVoltage8;
    
    String getJSON();
};
#endif