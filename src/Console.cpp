#include "Console.h"

void Console::printByteArray(byte buffer[])
{
    printByteArray(buffer, -1);
}

void Console::printByteArray(String prefix, byte buffer[])
{
    printByteArray(prefix, buffer, -1);
}

void Console::printByteArray(String prefix, byte buffer[], int len)
{
    Serial.print(prefix);
    if (len == -1)
    {
        int idx = 0;
        byte ch = buffer[idx];
        while (ch != 0x00)
        {
            if (idx > 0)
                m_channel->print(" ");

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
                m_channel->print(" ");
        }
    }

    m_channel->println(";");
}