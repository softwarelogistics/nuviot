#include "MessagePayload.h"

#include <Arduino.h>

String MessagePayload::getJSON()
{
    const size_t capacity = JSON_OBJECT_SIZE(14);
    DynamicJsonDocument doc(capacity);

    doc["status"] = status;
    doc["lastError"] = lastError;

    if(hasCurrent1) doc["current1"] = current1;
    if(hasCurrent2) doc["current2"] = current2;
    if(hasCurrent3) doc["current3"] = current3;
    
    if(hasHumidity1) doc["humidity1"] = humidity1;
    if(hasHumidity2) doc["humidity2"] = humidity2;
    if(hasHumidity3) doc["humidity3"] = humidity3;

    if(temperature1) doc["temperature1"] = temperature1;
    if(temperature2) doc["temperature2"] = temperature2;    
    if(temperature3) doc["temperature3"] = temperature3;

    if(hasVBatt) doc["vbatt"] = vbatt;
    if(hasVoltage1) doc["voltage1"] = voltage1;
    if(hasVoltage2) doc["voltage2"] = voltage2;
    if(hasVoltage3) doc["voltage3"] = voltage3;
    

    String output;
    serializeJson(doc, output);

    return output;
}