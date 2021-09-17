#ifndef CHANNEL_H
#define CHANNEL_H

#include <Arduino.h>
#include "Console.h"


#define TX_BUFFER_SIZE 2048
#define RX_BUFFER_SIZE 2048

class Channel
{
public:
    Channel(HardwareSerial *stream, Console *console);

    void clearBuffers();

    void transmit(String msg);
    void transmit(byte *msg, size_t len);
    void transmitln(String msg);

    void print(String msg);
    void println(String msg);

    void enqueueByte(uint8_t byte);
    void enqueueByteArray(uint8_t buffer[], size_t len);
    void enqueueString(String str);

    void setBaudRate(unsigned long baudRate);

    bool flush(bool transparentMode = false);
    
    uint16_t getEnqueuedLength(); 

    String readStringUntil(char ch, int timeout);

    size_t available();
    size_t readBytes(byte *buffer, size_t len);
    byte readByte();
    void loop();

    bool waitForCH(uint8_t ch);
    bool waitForCRLF();
    bool waitForCR();
    bool waitForLF();

private:
    int m_txHead = 0;
    int m_txTail = 0;

    int m_rxHead = 0;
    int m_rxTail = 0;

    byte m_txBuffer[TX_BUFFER_SIZE];
    byte m_rxBuffer[RX_BUFFER_SIZE];
    byte m_tempBuffer[1024];

    HardwareSerial *m_stream;
    Console *m_console;
};

#endif
