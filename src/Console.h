#ifndef CONSOLE_H
#define CONSOLE_H

#include <Arduino.h>

class Console
{
public:
    Console(Stream *stram);

    void printByte(byte byte);
    void printByte(String prefix, byte byte, String suffix);

    void printByteArray(byte buffer[]);
    void printByteArray(byte buffer[], size_t length);
    void printByteArray(String prefix, byte buffer[]);    
    void printByteArray(String prefix, byte buffer[], size_t length);
    
    void printVerbose(String msg);
    void print(String msg);
    void newline();
    void printError(String err);
    void printWarning(String warning);

    void setVerboseLogging(bool verbose);

private:
    Stream *m_stream;
    bool m_verboseLogging = false;
};

#endif
