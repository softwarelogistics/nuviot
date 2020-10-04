#include "Console.h"

Console::Console(Stream *stream)
{
    m_stream = stream;
    m_btSerial = NULL;
}

Console::Console(BluetoothSerial *btSerial, Stream *stream)
{
    m_stream = stream;
    m_btSerial = btSerial;
}

Console::Console(BluetoothSerial *btSerial)
{
    m_stream = NULL;
    m_btSerial = btSerial;
}

void Console::printByte(byte ch)
{
    if (m_verboseLogging)
    {

        char hexChar[2];

        if (m_stream != NULL && m_serialEnabled)
        {
            m_stream->print("0x");
            sprintf(hexChar, "%02X", ch);
            m_stream->print(hexChar);
        }

        if (m_btSerial != NULL && m_btEnabled)
        {
            m_btSerial->print("0x");
            sprintf(hexChar, "%02X", ch);
            m_btSerial->print(hexChar);
        }
    }
}

void Console::printByte(String prefix, byte byte, String suffix)
{
    if (m_verboseLogging)
    {
        print(prefix);
        printByte(byte);
        println(suffix);
    }
}

void Console::enableBTOut(bool enabled)
{
    m_btEnabled = enabled;
}

void Console::enableSerialOut(bool enabled)
{
    m_serialEnabled = enabled;
}

void Console::setVerboseLogging(bool verbose)
{
    m_verboseLogging = verbose;
}

void Console::printVerbose(String msg)
{
    if (m_verboseLogging)
    {
        println(msg);
    }
}

void Console::println(String msg)
{
    if (m_stream != NULL && m_serialEnabled)
    {
        m_stream->println(msg);
    }

    if (m_btSerial != NULL && m_btEnabled)
    {
        m_btSerial->println(msg);
    }
}

void Console::print(String msg)
{
    if (m_stream != NULL && m_serialEnabled)
    {
        m_stream->print(msg);
    }

    if (m_btSerial != NULL && m_btEnabled)
    {
        m_btSerial->print(msg);
    }
}

void Console::newline()
{
    if (m_stream != NULL && m_serialEnabled)
    {
        m_stream->println();
    }

    if (m_btSerial != NULL && m_btEnabled)
    {
        m_btSerial->println();
    }
}

void Console::printError(String err)
{
    if (m_stream != NULL && m_serialEnabled)
    {
        m_stream->println("[error]=>" + err);
    }

    if (m_btSerial != NULL && m_btEnabled)
    {
        m_btSerial->println("[error]=>" + err);
    }
}

void Console::printWarning(String warning)
{
    if (m_stream != NULL && m_serialEnabled)
    {
        m_stream->println("[warning]=>" + warning);
    }

    if (m_btSerial != NULL && m_btEnabled)
    {
        m_btSerial->println("[warning]=>" + warning);
    }
}

void Console::printByteArray(byte buffer[])
{
    printByteArray(buffer, -1);
}

void Console::printByteArray(byte buffer[], size_t len)
{
    printByteArray("", buffer, len);
}

void Console::printByteArray(String prefix, byte buffer[])
{
    printByteArray(prefix, buffer, -1);
}

void Console::printByteArray(String prefix, byte buffer[], size_t len)
{
    if (m_verboseLogging)
    {
        if (m_stream != NULL && m_serialEnabled)
        {
            m_stream->print(prefix);
        }

        if (len == -1)
        {
            int idx = 0;
            byte ch = buffer[idx];
            while (ch != 0x00)
            {
                if (idx > 0 && m_stream != NULL && m_serialEnabled)
                    m_stream->print(" ");

                printByte(ch);

                idx++;
                ch = buffer[idx];
            }
        }
        else
        {
            for (int idx = 0; idx < len; ++idx)
            {
                printByte(buffer[idx]);
                if (idx < len - 1 && m_stream != NULL && m_serialEnabled)
                    m_stream->print(" ");
            }
        }

        if (m_stream != NULL && m_serialEnabled)
        {
            m_stream->println(";");
        }
    }
}