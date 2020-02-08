#include "MessagePayload.h"

#include <Arduino.h>

void MessagePayload::generateSampleData()
{
    int x = (random(1, 2000) + 7000) / 100.0;
    int y = (random(1, 2000) + 5000) / 100.0;
    float compressor = random(0, 3);
    int high = random(0, 9);
    int low = random(0, 9);

    compressorCurrent = compressor * 4.21;
    highPressureRaw = high * 97 + 2430;
    highPressure = ((highPressureRaw - 442) * 174) / 3636;
    lowPressureRaw = low * 67 + 1230;

    lowPressure = ((lowPressureRaw - 442) * 174) / 3636;
    highTemperature = (random(1, 4000) + 7000) / 100.0;
    lowTemperature = (random(1, 2000) + 4000) / 100.0;

    int pan = random(0, 100);
    if (strcmp(panWater, "dry") == 0)
    {
        if (pan > 95)
            strcpy(panWater, "low");
    }
    else
    {
        if (pan > 90)
            strcpy(panWater, "dry");
    }
}

String MessagePayload::getJSON()
{
    const size_t capacity = JSON_OBJECT_SIZE(14);
    DynamicJsonDocument doc(capacity);

    // doc["timeStamp"] = timeStamp;
    if (isWeatherOnline)
    {
        doc["ambientTemperature"] = ambientTemperature;
        doc["ambientHumidity"] = ambientHumidity;
    }
    else
    {
        doc["weatherStatus"] = "offline";
    }

    if (hasCompressorCurrent)
    {
        doc["compressorCurrent"] = compressorCurrent;
    }
    else
    {
        doc["compressorCurrentStatus"] = "offline";
    }

    if (hasBlowerCurrent)
    {
        doc["blowerCurrent"] = blowerCurrent;
    }
    else
    {
        doc["blowerCurrentStatus"] = "offline";
    }

    if (areProbesOnline)
    {
        doc["highTemperature"] = highTemperature;
        doc["lowTemperature"] = lowTemperature;
    }
    else
    {
        doc["temperatureProbesStatus"] = "offline";
    }

    if (hasHighPressure)
    {
        doc["highPressureRaw"] = highPressureRaw;
        doc["highPressure"] = highPressure;
    }
    else
    {
        doc["highPressureStatus"] = "offline";
    }

    if (hasLowPressure)
    {
        doc["lowPressureRaw"] = lowPressureRaw;
        doc["lowPressure"] = lowPressure;
    }
    else
    {
        doc["lowPressureStatus"] = "offline";
    }
    
    if(hasPanStatus) 
    {
    doc["panWater"] = panWater;
    }
    else
    {
        doc["panStatus"] = "offline";
    }
    
    String output;
    serializeJson(doc, output);

    return output;
}