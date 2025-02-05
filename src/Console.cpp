#include "Console.h"

Console::Console(Stream *stream)
{
    m_stream = stream;

    for (int idx = 0; idx < CONSOLE_IN_BUFFER_LEN; ++idx)
    {
        m_consoleInBuffer[idx] = 0;
    }
}

void Console::printByte(byte ch)
{
    if (m_verboseLogging)
    {
        char hexChar[2];

        print("0x");
        sprintf(hexChar, "%02X", ch);
        print(hexChar);
    }
}

void Console::loop(){
    if(!m_serialEnabled) 
        return;

    int bytesToRead = m_stream->available();
    if (bytesToRead)
    {
        for (int idx = 0; idx < bytesToRead; ++idx)
        {
            int ch = m_stream->read();
            
            if (ch == '\n')
            {
                String cmd = String(m_consoleInBuffer);
                println(cmd);
                newline();

                if (m_callback != NULL)
                {
                    m_callback(cmd);
                }

                for (int idx = 0; idx < CONSOLE_IN_BUFFER_LEN; ++idx)
                {
                    m_consoleInBuffer[idx] = 0;
                    m_consoleInBufferIdx = 0;
                }
            }
            else
            {
                m_consoleInBuffer[m_consoleInBufferIdx++] = ch;
            }
        }
    }
}

void Console::printByte(String prefix, byte byte, String suffix)
{
    if (m_verboseLogging)
    {
        print(prefix);
        printByte(byte);
        print(suffix);
        newline();
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

bool Console::getVerboseLogging()
{
    return m_verboseLogging;
}

void Console::printVerbose(String msg)
{
    if (m_verboseLogging)
    {
        print(msg);
        newline();
    }
}

void Console::repeatFatalError(String error)
{
    println("[fatalerror]=>" + error);
    long nextSend = millis() + 1000;
    while (true)
    {
        if (nextSend < millis())
        {
            println("[fatalerror]=>" + error);
            nextSend = millis() + 1000;
        }
    }
}

void Console::println(String msg)
{
    print(msg);
    newline();
}

void Console::print(String msg)
{
    if (m_stream != NULL && m_serialEnabled)
    {
        m_stream->print(msg);
    }

    if (m_bleCallback != NULL && m_btEnabled)
    {
        if(msg.length() > 500) {
            m_bleCallback(msg.substring(0, 500));
        }
        else 
            m_bleCallback(msg);
    }
}

void Console::newline() { print("\r\n"); }

void Console::printError(String err)
{
    print("[error] ");
    println(err);
}

void Console::printWarning(String warning)
{
    print("[warning] ");
    println(warning);
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
        print(prefix);

        if (len == -1)
        {
            int idx = 0;
            byte ch = buffer[idx];
            while (ch != 0x00)
            {
                if (idx > 0)
                    print(" ");

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
                    print(" ");
            }
        }

        println(";");
    }
}