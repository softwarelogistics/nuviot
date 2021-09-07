#ifndef CONSOLE_H
#define CONSOLE_H

#include <Arduino.h>
#include <Stream.h>
#define CONSOLE_IN_BUFFER_LEN 128
#define CONSOLE_OUT_BUFFER 2048

class Console
{
private:    
    bool m_btEnabled = true;
    bool m_serialEnabled = true;
    int m_consoleInBufferIdx = 0;
    char m_consoleInBuffer[CONSOLE_IN_BUFFER_LEN];
    char m_consoleOutBuffer[CONSOLE_OUT_BUFFER];
    void (*m_callback)(String topic) = NULL;
    void (*m_bleCallback)(String topic) = NULL;

    uint16_t m_consoleOutBufferHead = 0;
    uint16_t m_consoleOutBufferTail = 0;

public:    
    Console(Stream *stream);

    void printByte(byte byte);
    void printByte(String prefix, byte byte, String suffix);

    void printByteArray(byte buffer[]);
    void printByteArray(byte buffer[], size_t length);
    void printByteArray(String prefix, byte buffer[]);    
    void printByteArray(String prefix, byte buffer[], size_t length);

    void repeatFatalError(String err);

    void printVerbose(String msg);
    void println(String msg);
    void print(String msg);
    void newline();
    void printError(String err);
    void printWarning(String warning);

    void setVerboseLogging(bool verbose);
    bool getVerboseLogging();

    void enableBTOut(bool enabled);
    void enableSerialOut(bool enabled);
    void registerCallback(void (*callback)(String topic)) {
        m_callback = callback;
    }

    void registerBLEWriter(void (*callback)(String message)) {
        m_bleCallback = callback;
    }

    void loop();

private:
    Stream *m_stream;
    bool m_verboseLogging = false;
};

#endif
