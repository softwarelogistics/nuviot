#include <Arduino.h>
#include "MQTT_GPRS.h"

MQTT::MQTT(Channel *channel, Console *console)
{
    m_channel = channel;
    m_console = console;
}

void MQTT::reset()
{
    m_rxBufferHead = 0;
    m_rxBufferTail = 0;

    m_txBufferHead = 0;
    m_txBufferTail = 0;

    m_lastError = "NOERROR";
    m_subscriptionId = 0;
    m_packetId = 0;

    m_channel->clearBuffers();
}

void MQTT::writeControlField(byte control_field)
{
    m_channel->enqueueByte(control_field);
}

void MQTT::writeRemainingLength(unsigned int remainingLength)
{
    m_console->printVerbose("Real Length: [" + String(remainingLength) + "] total packet length: [" + String(remainingLength + 2) + "]");

    // In the MQTT spec if longer then 127 set bit 8 in the first byte
    // to send over, set the continuation bit
    // Then take what's in the bits after the 7th bit
    // shift them over so they make up byte of 7 bits.
    //
    // NOTE: Right now this is limited to a 16 bit int, probably
    //       realisitc size for a device with this amount of memory
    //       could always take in a long shift over the length
    //       until value is zero to handle larger values.
    //
    if (remainingLength > 0x7F)
    {
        byte lsb = ((remainingLength & 0x7F) | 0x80);
        m_channel->enqueueByte(lsb);
        byte msb = ((remainingLength >> 7) & 0xFF);
        m_channel->enqueueByte(msb);
    }
    else
    {
        m_channel->enqueueByte((uint8_t)remainingLength);
    }
}

void MQTT::writeLengthPrefixedString(String str)
{
    byte lenBuffer[2];
    lenBuffer[0] = (byte)(str.length() >> 8);
    lenBuffer[1] = (byte)(str.length() & 0xFF);
    m_channel->enqueueByteArray(lenBuffer, 2);
    m_channel->enqueueString(str);

    m_console->printVerbose("Sending [" + String(str.length() + 2) + "] bytes for [" + str + "]");
}

void MQTT::writeString(String str)
{
    m_channel->enqueueString(str);
}

void MQTT::writeByteArray(byte *buffer, int length)
{
    m_channel->enqueueByteArray(buffer, length);
}

bool MQTT::flush()
{
    if(!m_transparentMode) {
        m_console->printVerbose("NOT TRANSPARENT MODE - CIPSEND");

        uint16_t enqueuedBytes = m_channel->getEnqueuedLength();
        String sendMessage = "AT+CIPSEND=" + String(enqueuedBytes);
        m_channel->println(sendMessage);

        String response = m_channel->readStringUntil('\n', 3000);

        m_console->printVerbose("AT+CIPSEND= response: " + response);

        uint8_t ch = 0x00;
        uint16_t retryCount = 0;
        while(ch != '>' && retryCount++ < 500)
        {
            while(m_channel->available() > 0 && ch != '>') {
                ch = m_channel->readByte();
                m_console->printVerbose("UNEXPECTED RESPONSE: [" + String(ch) + "]");
            }
            delay(1);
        } 

        if(ch != '>')
        {
            m_console->printError("mqttflush=fail; //timeout waiting for > ");
            return false;
        }

        m_console->printVerbose("RECEIVED: [" + String(ch) + "] Will continue");
    }
    else {
        m_console->printVerbose("TRANSPARENT MODE - NO CIPSEND");
    }

    if(!m_channel->flush())
    {
        m_console->printError("mqttflush=fail; // could not flush channel");
        return false;
    }

    String response = m_channel->readStringUntil('\n', 3000);
    m_console->printVerbose(response);
    response = m_channel->readStringUntil('\n', 3000);
    m_console->printVerbose(response);

    return true;
}

int MQTT::readRemainingLength()
{
    int remainingLength = m_channel->readByte();

    return remainingLength;
}

void MQTT::handlePublishedMessage()
{
    int remainingLength = readRemainingLength();
    if (remainingLength > 0)
    {
        size_t bytesRead = m_channel->readBytes(m_rxBuffer, remainingLength);
        if(bytesRead != remainingLength)
        {
            m_console->println("mqtthandlepublish=false; // invalid message size");
            return;
        }

        int topicLength = (m_rxBuffer[0] << 8) | m_rxBuffer[1];
        String topic = "";
        for (int idx = 0; idx < topicLength; ++idx)
        {
            // there is likely a better way...
            topic += String((char)m_rxBuffer[idx + 2]);
        }

        if (messageReceivedCallback != NULL)
        {
            messageReceivedCallback(topic, &m_rxBuffer[2 + topicLength], remainingLength - (2 + topicLength));
        }
    }
}

void MQTT::checkForReceivedMessages()
{
    byte responseCode = m_channel->readByte();
    m_console->printByte("Received [", responseCode, "] from MQTT Server.");
    bool isVerbose = m_console->getVerboseLogging();
    m_console->setVerboseLogging(true);
    m_console->println("modem=unsolicitedresponse; // byte [" + String(responseCode) + "]");
    m_console->setVerboseLogging(isVerbose);

    switch (responseCode)
    {
    case MQTT_PUBLISH:
        handlePublishedMessage();
        case 0x0d:
            String response = m_channel->readStringUntil(0x0a, 250);
            response.trim();
            if(response.length() == 0)
            {
                response = m_channel->readStringUntil(0x0a, 250);
                response.trim();
                m_console->println("modem=unsolicitedresponse;" + response);
                if(response == "CLOSED")
                {
                    m_console->println("modem=connectionlost;");
                    m_closed = true;
                }
            }
            break;

    }
}

bool MQTT::readResponse(byte expected)
{
    int loopCount = 200;
    m_console->printByte("Waiting for [", expected, "] as response from MQTT Server.");

    while (loopCount-- > 0)
    {
        if (m_channel->available() > 0)
        {
            byte responseCode = m_channel->readByte();
            m_console->printByte("Received [", responseCode, "] from MQTT Server.");

            switch (responseCode)
            {
            case MQTT_PUBLISH:
                handlePublishedMessage();
                break;
            default:
                if (responseCode == expected)
                {
                    m_console->printVerbose("Returned expected response code.");
                    return true;
                }

                int remainingLength = readRemainingLength();
                if (remainingLength > 0)
                {
                    m_console->printVerbose("Returned remaining length: " + String(remainingLength));
                    size_t bytesRead = m_channel->readBytes(m_rxBuffer, remainingLength);
                    if (bytesRead == remainingLength)
                    {
                        m_console->printByteArray(m_rxBuffer, remainingLength);
                    }
                    else
                    {
                        m_console->printError("readresponse=fail; // Expected RL of " + String(remainingLength) + " received " + String((int)bytesRead) + " bytes.");
                        return false;
                    }
                }
                else
                {
                    m_console->printVerbose("No payload from message.  ");
                }
            }
        }

        delay(10);
    }

    m_console->printError("readresponse=failed; // timeout waiting for response from server.");

    return false;
}

bool MQTT::connect(String uid, String pwd, String clientId)
{
    m_console->setVerboseLogging(true);

    byte connectFlag = 0x03;

    bool isAuth = uid.length() > 0 && pwd.length() > 0;

    String MQTT_HEADER = "MQTT";

    int remainingLength = 2 + MQTT_HEADER.length() + // MQTT String Plus length
                          1 + 1 + 2 +                //  0x40, 0x02, 0x00, 0x3C
                          2 + clientId.length();

    if (isAuth)
    {
        remainingLength += 2 + uid.length() + 2 + pwd.length();
        connectFlag = 0xC2;
    }

    byte mqttHeader[] = {
        0x04,        // Protocol Version
        connectFlag, // Connect Flags
        0x00, 0x3C,  // Keep Alive
    };

    m_channel->clearBuffers();

    writeControlField(MQTT_CONNECT);
    writeRemainingLength(remainingLength);

    writeLengthPrefixedString(MQTT_HEADER);

    writeByteArray(mqttHeader, sizeof(mqttHeader));

    writeLengthPrefixedString(clientId);
    if (isAuth)
    {
        writeLengthPrefixedString(uid);
        writeLengthPrefixedString(pwd);
    }

    if(!flush())
    {
        return false;
    }

    if (readResponse(0x20))
    {
        m_closed = false;
        m_console->printVerbose("mqttconnect=success;");
        m_console->setVerboseLogging(true);
        return true;
    }
    else
    {
        m_console->printError("mqttconnect=fail; // invalid response.");
        m_console->setVerboseLogging(true);
        return false;
    }
}

bool MQTT::disconnect()
{
    return true;
}

bool MQTT::publish(String topic, String payload, byte qos)
{
    int rl = 2 + topic.length() +
             (qos == QOS0 ? 0 : 2) + // packet id
             payload.length();

    byte controlField = MQTT_PUBLISH;

    controlField = controlField | (qos << 1);

    m_channel->clearBuffers();

    writeControlField(controlField);
    writeRemainingLength(rl);

    writeLengthPrefixedString(topic);
    if(qos > QOS0)
    {
        m_channel->enqueueByte((uint8_t)((m_packetId >> 8) && 0xFF));
        m_channel->enqueueByte((uint8_t)(m_packetId && 0xFF));

        m_packetId++;
    }

    writeString(payload);

    if(!flush()) return false;   

    return (qos > QOS0) ? readResponse(0x40) : true;
}

bool MQTT::publish(String topic, byte buffer[], uint16_t len, byte qos) {
    int rl = 2 + topic.length() +
             (qos == QOS0 ? 0 : 2) + // packet id
             len;

    byte controlField = MQTT_PUBLISH;

    controlField = controlField | (qos << 1);

    m_channel->clearBuffers();

    writeControlField(controlField);
    writeRemainingLength(rl);

    writeLengthPrefixedString(topic);

    if(qos > QOS0) 
    {
        m_channel->enqueueByte((uint8_t)((m_packetId >> 8) && 0xFF));
        m_channel->enqueueByte((uint8_t)(m_packetId && 0xFF));
        m_packetId++;
    }

    writeByteArray(buffer, len);
    
    if(!flush()) return false;

    return (qos > QOS0) ? readResponse(0x40) : true;
}


bool MQTT::publish(String topic, byte qos)
{
    byte rl = 2 + topic.length() + 2; // packet id

    byte controlField = MQTT_PUBLISH;

    controlField = controlField | (qos << 1);

    m_channel->clearBuffers();

    writeControlField(controlField);
    writeRemainingLength(rl);

    writeLengthPrefixedString(topic);

    m_channel->enqueueByte((uint8_t)((m_packetId >> 8) && 0xFF));
    m_channel->enqueueByte((uint8_t)(m_packetId && 0xFF));

    m_packetId++;

    if(!flush()) return false;

    return (qos > QOS0) ? readResponse(0x40) : true;
}

byte MQTT::subscribe(String topic, byte qos)
{
    byte payloadLen = 2 + 2 + topic.length() + 1;

    byte subscribeHeader[] = {
        0x82,
        payloadLen,
        0x00, m_subscriptionId};

    m_channel->clearBuffers();

    writeByteArray(subscribeHeader, sizeof(subscribeHeader));
    writeLengthPrefixedString(topic);

    byte qosBuffer[] = {
        qos,
    };

    writeByteArray(qosBuffer, 1);
    if(!flush()) return -1;

    if (readResponse(0x90))
    {
        m_subscriptionId++;
        m_console->printVerbose("Subscribed to [" + topic + "]");
        /* we previously incremented it so decrement it now */
        return m_subscriptionId - 1;
    }
    else
    {
        m_console->printError("Could not subscribe to [" + topic + "]");
        return -1;
    }

    return -1;
}

bool MQTT::ping()
{
    byte pingMsg[] = {0xC0, 0x00};

    m_channel->clearBuffers();

    writeByteArray(pingMsg, sizeof(pingMsg));
    if(!flush()) return false;

    return readResponse(0xD0);
}

void MQTT::setMessageReceivedCallback(void (*callback)(String topic, unsigned char *buffer, size_t len))
{
    messageReceivedCallback = callback;
}

void MQTT::loop()
{
    if (m_channel->available() > 0)
    {
        checkForReceivedMessages();
    }
}