#ifndef CONSOLE_H
#define CONSOLE_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#define CONSOLE_IN_BUFFER_LEN 128

class Console
{
private:
    
    bool m_btEnabled = true;
    bool m_serialEnabled = true;
    int m_consoleInBufferIdx = 0;
    char m_consoleInBuffer[CONSOLE_IN_BUFFER_LEN];
    void (*m_callback)(String topic) = NULL;

public:
    Console(Stream *stream);
    Console(BluetoothSerial *btSerial, Stream *stream);
    Console(BluetoothSerial *btSerial);

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
     void registerCallback(void (*callback)(String topic));

    void loop();

private:
    BluetoothSerial *m_btSerial = NULL;
    Stream *m_stream;
    bool m_verboseLogging = false;
};

#endif
