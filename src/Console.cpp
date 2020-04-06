#include "Console.h"

Console::Console(Stream *stream)
{
    m_stream = stream;
}

void Console::printByte(byte ch)
{
    if (m_verboseLogging)
    {
        char hexChar[2];
        m_stream->print("0x");
        sprintf(hexChar, "%02X", ch);
        m_stream->print(hexChar);
    }
}

void Console::printByte(String prefix, byte byte, String suffix)
{
    if (m_verboseLogging)
    {
        m_stream->print(prefix);
        printByte(byte);
        m_stream->println(suffix);
    }
}

void Console::setVerboseLogging(bool verbose)
{
    m_verboseLogging = verbose;
}

void Console::printVerbose(String msg)
{
    if (m_verboseLogging)
    {
        m_stream->println(msg);
    }
}

void Console::print(String msg)
{
    m_stream->println(msg);
}

void Console::newline()
{
    m_stream->println();
}

void Console::printError(String err)
{
    m_stream->println("ERROR");
    m_stream->println("=======");
    m_stream->println(err);
    m_stream->println();
}

void Console::printWarning(String warning)
{
    m_stream->println("WARNING");
    m_stream->println("=======");
    m_stream->println(warning);
    m_stream->println();
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
        m_stream->print(prefix);
        if (len == -1)
        {
            int idx = 0;
            byte ch = buffer[idx];
            while (ch != 0x00)
            {
                if (idx > 0)
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
                if (idx < len - 1)
                    m_stream->print(" ");
            }
        }

        m_stream->println(";");
    }
}