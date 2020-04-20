#include "Channel.h"

Channel::Channel(HardwareSerial *stream, Console *console)
{
    m_stream = stream;
    m_console = console;
}

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
    long start = millis();
    while(m_stream->available() < length && ((millis() - start) < 5000));        

    if(m_stream->available() < length){
        return -1;
    }

    return m_stream->readBytes(buffer, length);
}

void Channel::transmit(String msg)
{
    m_stream->print(msg);
}

void Channel::transmit(byte *buffer, size_t len)
{
    m_stream->write(buffer, len);
}

void Channel::transmitln(String msg)
{
    m_stream->println(msg);
}

void Channel::print(String msg)
{
    m_stream->print(msg);
}

void Channel::println(String msg)
{
    m_stream->println(msg);
}

void Channel::setBaudRate(unsigned long baudRate) {
    m_stream->flush();    
    m_stream->end();
    m_stream->updateBaudRate(baudRate);
    delay(1000);

    m_console->printVerbose("We setting baud rate on stream " + String(baudRate) + " baud rate " + String(m_stream->baudRate()));
}

String Channel::readStringUntil(char ch, int timeout)
{
    unsigned long existingTimeout = m_stream->getTimeout();
    m_stream->setTimeout(timeout);
    String result = m_stream->readStringUntil(ch);
    m_stream->setTimeout(existingTimeout);
    return result;
}

void Channel::enqueueString(String str){
    enqueueByteArray((uint8_t *)str.c_str(), str.length());
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

    m_txHead = m_txTail;
}