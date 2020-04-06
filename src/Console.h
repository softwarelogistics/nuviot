#ifndef CONSOLE_H
#define CONSOLE_H

#include <Arduino.h>
#include "Channel.h"

class Console
{
public:
    Console(Channel *stream);

    void printByte(byte byte);
    void printByte(String prefix, byte byte, String suffix);
    void printByteArray(byte buffer[]);
    void printByteArray(String prefix, byte buffer[]);
    void printByteArray(byte buffer[], int length);
    void printByteArray(String prefix, byte buffer[], int length);
    void printVerbose(String msg);
    void print(String msg);
    void printError(String err);
    void printWarning(String warning);

private:
    Channel *m_channel;
};

#endif
