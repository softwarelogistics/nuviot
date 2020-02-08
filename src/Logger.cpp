#include "Logger.h"

void Logger::setup() 
{
    Serial.begin(115200);  
}

void Logger::loop() 
{
    
}

void Logger::logVerbose(String str)
{
    Serial.println(str);
    delay(250);
}

void Logger::logWarning(String str)
{
    Serial.println(str);
    delay(250);
}

void Logger::logError(String str)
{
    Serial.println(str);
    delay(250);
}