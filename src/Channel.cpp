#include "Channel.h"

void Channel::enqueueByte(uint8_t byte)
{
    m_txBuffer[m_txTail] = byte;
    m_txTail++;
    if (m_txTail == TX_BUFFER_SIZE)
    {
        m_txTail = 0;
    }
}

byte Channel::readByte() 
{
    return (byte)m_stream->read();
}

size_t Channel::available()
{
    return m_stream->available();
}

size_t Channel::readBytes(byte *buffer, size_t length)
{
    return m_stream->readBytes(buffer, length);
}

void Channel::transmit(String msg)
{
    m_stream->print(msg);
}

void Channel::transmit(byte *buffer, size_t len)
{
    m_stream->write((char *)buffer, len);
}

void Channel::transmitln(String msg)
{
    m_stream->print(msg);
}

void Channel::print(String msg)
{
    m_stream->print(msg);
}

void Channel::println(String msg)
{
    m_stream->println(msg);
}

void Channel::enqueueByteArray(uint8_t buffer[], size_t len)
{
    for (int idx = 0; idx < len; ++idx)
    {
        enqueueByte(buffer[idx]);
    }
}

void Channel::flush() {
     int sendLength = m_txTail - m_txHead;

    if (m_txTail < m_txHead)
    {
        for (int idx = m_txHead; idx < TX_BUFFER_SIZE; ++idx)
        {
            m_stream->write(m_txBuffer[idx]);
        }

        for (int idx = 0; idx < m_txTail; ++idx)
        {
            m_stream->write(m_txBuffer[idx]);
        }

        int sendLength = m_txTail + (TX_BUFFER_SIZE - m_txHead);
    }
    else
    {
        for (int idx = m_txHead; idx < m_txTail; ++idx)
        {
            m_stream->write(m_txBuffer[idx]);
        }
    }

    // The SIM module will echo back all the bytes are sent to it.
    // this simply pulls them from the receive buffer so they don't
    // get in the way of the other algorithms.
    while (m_stream->available() < sendLength)
        ;

    int charCount = m_stream->readBytes(m_tempBuffer, sendLength);
    if(charCount != sendLength)
    {
        Serial.println("WTF!!!! Didn't read correct amount.");
    }

    for(int idx = 0; idx < charCount; ++idx){
        if(idx > 0)
            Serial.print(" ");
            
        m_console->printByte(m_tempBuffer[idx + m_txHead]);
    }

    Serial.println(";");

    for(int idx = 0; idx < charCount; ++idx){
        if(idx > 0)
            Serial.print(" ");

        m_console->printByte(m_txBuffer[idx + m_txHead]);
    }

    Serial.println(";");

    m_txHead = m_txTail;
}