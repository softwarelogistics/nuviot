#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include "Console.h"
#include "Channel.h"

#define MQTT_CONNECT 0x10
#define MQTT_PUBLISH 0x30
#define MQTT_SUBSCRIBE 0x82

#define QOS0 0
#define QOS1 1
#define QOS2 2

class MQTT {
public:
    MQTT(Channel *stream, Console *console);
   
    bool connect(String uid, String pwd, String clientId);
    bool disconnect();
    bool publish(String topic, String payload, byte qos);
    bool publish(String topic, byte buffer[], uint16_t len, byte qos);
    bool publish(String topic, byte qos);
    byte subscribe(String topic, byte qos);
    bool ping();
    void setMessageReceivedCallback(void (*callback)(String topic, byte buffer[], size_t buffer_length));
    void setErrorCallback(void (*callback)(String error));      
    void loop();
    void reset();

    bool getIsClosed() {return m_closed;}

    void setTransparentMode(bool transparentMode) {
        m_transparentMode = transparentMode;
    }

    String getLastError();

private:
    void (*errorCallback)(String error) = NULL;
    void (*messageReceivedCallback)(String topic, unsigned char buffer[], size_t length) = NULL;

    Console *m_console;
    Channel *m_channel;
    String m_lastError;

    bool m_closed = true;

    byte m_rxBuffer[4096];
    byte m_txBuffer[4096];

    int m_rxBufferHead = 0;
    int m_rxBufferTail = 0;

    int m_txBufferHead = 0;
    int m_txBufferTail = 0;

    int m_packetId = 0;
    byte m_subscriptionId = 0;

    Stream *m_stream;

    bool m_transparentMode = false;

    void writeString(String string);
    void writeLengthPrefixedString(String str);
    void writeControlField(byte control_field);
    void writeRemainingLength(unsigned int remaining_length);
    int readRemainingLength();
    void writeByteArray(byte *buffer, int length);
    void handlePublishedMessage(byte qos);

    void checkForReceivedMessages();

    bool flush();

    bool readResponse(byte expected);
};

#endif
