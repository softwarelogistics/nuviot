#include "MQTT.h"

MQTT::MQTT(Channel *channel, Console *console)
{
    m_channel = channel;
    m_console = console;
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
        byte lsb = remainingLength & 0x7F | 0x80;
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
    m_channel->transmit(lenBuffer, 2);
    m_channel->print(str);

    m_console->printVerbose("Sending [" + String(str.length() + 2) + "] bytes for [" + str + "]");
}

void MQTT::writeString(String str)
{
    m_channel->print(str);
}

void MQTT::writeByteArray(byte *buffer, int length)
{
    m_channel->enqueueByteArray(buffer, length);
}

void MQTT::flush()
{
    m_channel->flush();
}

int MQTT::readRemainingLength()
{
    int remainingLength = m_channel->readByte();

    return remainingLength;
}

byte MQTT::readResponse(byte expected)
{
    int loopCount = 10;
    m_console->printByte("Waiting for [", expected, "] as response from MQTT Server.");

    while (loopCount-- > 0)
    {

        if (m_channel->available() > 0)
        {
            byte responseCode = m_channel->readByte();
            int remainingLength = readRemainingLength();
            if (remainingLength > 0)
            {
                m_channel->readBytes(m_rxBuffer, remainingLength);
            }

            if (responseCode == expected)
            {
                return true;
            }
        }

        delay(100);
    }

    m_console->printError("Error waiting for MQTT Response.");

    return false;
}

bool MQTT::connect(String uid, String pwd, String clientId)
{
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

    flush();

    if (readResponse(0x20))
    {
        m_console->print("Connected to MQTT");
        return true;
    }
    else
    {
        m_console->print("Not connect to MQTT.");
        return false;
    }
}

bool MQTT::disconnect()
{
}

bool MQTT::publish(String topic, String payload, byte qos)
{
    int rl = 2 + topic.length() +
             2 + // packet id
             payload.length();

    byte controlField = MQTT_PUBLISH;

    controlField = controlField | (qos << 1);

    writeControlField(controlField);
    writeRemainingLength(rl);

    writeLengthPrefixedString(topic);

    m_channel->enqueueByte((uint8_t)((m_packetId >> 8) && 0xFF));
    m_channel->enqueueByte((uint8_t)(m_packetId && 0xFF));

    writeString(payload);

    flush();

    return (qos > QOS0) ? readResponse(0x40) : true;
}

bool MQTT::publish(String topic, byte qos)
{
    byte rl = 2 + topic.length() +
              2; // packet id

    byte controlField = MQTT_PUBLISH;

    controlField = controlField | (qos << 1);

    writeControlField(controlField);
    writeRemainingLength(rl);

    writeLengthPrefixedString(topic);

    m_channel->enqueueByte((uint8_t)((m_packetId >> 8) && 0xFF));
    m_channel->enqueueByte((uint8_t)(m_packetId && 0xFF));

    m_packetId++;

    flush();

    return (qos > QOS0) ? readResponse(0x40) : true;
}

bool MQTT::subscribe(String topic, byte qos)
{
    byte payloadLen = 2 + 2 + topic.length() + 1;

    byte subscribeHeader[] = {
        0x82,
        payloadLen,
        0x00, m_subscriptionId};

    writeByteArray(subscribeHeader, sizeof(subscribeHeader));
    writeLengthPrefixedString(topic);

    byte qosBuffer[] = {
        qos,
    };

    writeByteArray(qosBuffer, 1);
    flush();

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

    writeByteArray(pingMsg, sizeof(pingMsg));
    flush();

    return readResponse(0xD0);
}

void MQTT::setMessageReceivedCallback(void (*callback)(String topic, unsigned char *buffer, size_t len))
{
}

void MQTT::loop()
{
}