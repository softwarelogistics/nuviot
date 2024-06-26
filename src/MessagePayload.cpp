#include "MessagePayload.h"

#include <Arduino.h>

String MessagePayload::getJSON()
{
    JsonDocument doc;

    doc["status"] = status;
    doc["lastError"] = lastError;

    if(hasCurrent1) doc["c1"] = current1;
    if(hasCurrent2) doc["c2"] = current2;
    if(hasCurrent3) doc["c3"] = current3;
    
    if(hasHumidity1) doc["h1"] = humidity1;
    if(hasHumidity2) doc["h2"] = humidity2;
    if(hasHumidity3) doc["h3"] = humidity3;
    if(hasHumidity4) doc["h4"] = humidity3;

    if(temperature1) doc["t1"] = temperature1;
    if(temperature2) doc["t2"] = temperature2;    
    if(temperature3) doc["t3"] = temperature3;
    if(temperature4) doc["t4"] = temperature4;

    if(hasVoltage1) doc["v1"] = voltage1;
    if(hasVoltage2) doc["v2"] = voltage2;
    if(hasVoltage3) doc["v3"] = voltage3;
    if(hasVoltage4) doc["v4"] = voltage4;
    if(hasVoltage5) doc["v5"] = voltage5;
    if(hasVoltage6) doc["v6"] = voltage6;    
    if(hasVoltage7) doc["v7"] = voltage7;    
    if(hasVoltage8) doc["v7"] = voltage8;    
    
    String output;
    serializeJson(doc, output);

    return output;
}