#include "Utils.h"

bool isString(byte buffer[], int len)
{
    for (int idx = 0; idx < len; ++idx)
    {
        if ((buffer[idx] < 32 || buffer[idx] > 127) &&
            buffer[idx] != 0x00 &&
            buffer[idx] != 0x0d && buffer[idx] != 0x0a)
        {
            return false;
        }
    }

    return true;
}