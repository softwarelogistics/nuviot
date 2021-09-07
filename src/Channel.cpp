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
    while (m_stream->available() < length && ((millis() - start) < 5000))
        ;

    if (m_stream->available() < length)
    {
        size_t bufflen = m_stream->available();
        m_console->printError("channelreadbyte:failed; // Expected: " + String(length) + ", Actual " + String(bufflen));
        m_stream->readBytes(buffer, bufflen);
        bool isVerbose = m_console->getVerboseLogging();
        m_console->setVerboseLogging(true);
        m_console->printByteArray(buffer, bufflen);
        m_console->setVerboseLogging(isVerbose);
        return -1;
    }

    return m_stream->readBytes(buffer, length);
}

void Channel::clearBuffers()
{
    bool isVerbose = m_console->getVerboseLogging();
    m_console->setVerboseLogging(true);
    size_t bufflen = m_stream->available();
    m_stream->readBytes(m_rxBuffer, bufflen);
    m_console->printByteArray(m_rxBuffer, bufflen);

    m_console->setVerboseLogging(isVerbose);

    m_txHead = 0;
    m_txTail = 0;
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

bool Channel::waitForCH(uint8_t ch){
    while (!m_stream->available())
        ;
    if(m_stream->available())
    {
        return m_stream->read() == ch;
    }
    return false;
}

bool Channel::waitForCRLF()
{
    if (waitForCR())
    {
        return waitForLF();
    }

    return false;
}

bool Channel::waitForCR()
{
    return waitForCH('\r');
}

bool Channel::waitForLF()
{
    return waitForCH('\n');
}

void Channel::setBaudRate(unsigned long baudRate)
{
    m_stream->flush();
    m_stream->end();
    m_stream->updateBaudRate(baudRate);
    delay(1000);
}

String Channel::readStringUntil(char ch, int timeout)
{
    unsigned long existingTimeout = m_stream->getTimeout();
    m_stream->setTimeout(timeout);
    String result = m_stream->readStringUntil(ch);
    m_stream->setTimeout(existingTimeout);
    return result;
}

void Channel::enqueueString(String str)
{
    enqueueByteArray((uint8_t *)str.c_str(), str.length());
}

void Channel::enqueueByteArray(uint8_t buffer[], size_t len)
{
    for (int idx = 0; idx < len; ++idx)
    {
        enqueueByte(buffer[idx]);
    }
}

uint16_t Channel::getEnqueuedLength()
{
    if (m_txTail < m_txHead)
    {
        return m_txTail + (TX_BUFFER_SIZE - m_txHead);
    }
    else
    {
        return m_txTail - m_txHead;
    }
}

bool Channel::flush()
{
    uint16_t sendLength;
    uint16_t bytesWritten = 0;

    if (m_txTail < m_txHead)
    {
        for (int idx = m_txHead; idx < TX_BUFFER_SIZE; ++idx)
        {
            bytesWritten += m_stream->write(m_txBuffer[idx]);
        }

        for (int idx = 0; idx < m_txTail; ++idx)
        {
            bytesWritten += m_stream->write(m_txBuffer[idx]);
        }

        sendLength = m_txTail + (TX_BUFFER_SIZE - m_txHead);
    }
    else
    {
        sendLength = m_txTail - m_txHead;
        for (int idx = m_txHead; idx < m_txTail; ++idx)
        {
            bytesWritten += m_stream->write(m_txBuffer[idx]);
        }
    }

    m_txHead = m_txTail;

    if (sendLength != bytesWritten)
    {
        m_console->printError("channelflush=failed; // send/write mismatch, Sent:" + String(sendLength) + " Written" + String(bytesWritten) + ".");
        return false;
    }
    else
    {
        m_console->printVerbose("channelflush=success; // send/wrote " + String(sendLength));
    }

    return true;
}