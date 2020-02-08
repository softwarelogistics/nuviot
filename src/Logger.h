#ifndef Logger_c
#define Logger_c

#include <Arduino.h>

class Logger
{
public:
    void setup();
    void loop();
    void logVerbose(String str);
    void logWarning(String str);
    void logError(String str);
};

#endif
