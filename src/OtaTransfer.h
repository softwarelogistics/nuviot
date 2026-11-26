#ifndef NUVIOT_OTA_TRANSFER_H
#define NUVIOT_OTA_TRANSFER_H

#include <stddef.h>
#include <stdint.h>

namespace OtaTransfer
{
    // Legacy endpoint returns a decimal int. Parse without signed wrap or truncation.
    inline bool parseDecimal(const char *text, uint32_t limit, uint32_t &value)
    {
        if (!text) return false;
        while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') ++text;
        uint32_t result = 0;
        bool digit = false;
        while (*text >= '0' && *text <= '9')
        {
            uint32_t next = static_cast<uint32_t>(*text++ - '0');
            if (next > limit || result > (limit - next) / 10) return false;
            result = result * 10 + next;
            digit = true;
        }
        while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') ++text;
        if (!digit || *text) return false;
        value = result;
        return true;
    }

    enum Result { Complete, InvalidSize, ReadFailed, WriteFailed };

    // Every retry must fetch the entire block before any flash write.
    // A short flash write is fatal: its side effects cannot safely be retried.
    template <typename Fetch, typename Write, typename Feed>
    Result transfer(uint32_t total, uint8_t *buffer, size_t capacity,
                    Fetch fetch, Write write, Feed feed)
    {
        if (!total || total > INT32_MAX || !buffer || !capacity || capacity > INT32_MAX)
            return InvalidSize;
        uint32_t offset = 0;
        while (offset < total)
        {
            size_t length = total - offset;
            if (length > capacity) length = capacity;
            bool received = false;
            for (unsigned attempt = 0; attempt < 5; ++attempt)
            {
                feed();
                if (fetch(offset, buffer, length) == static_cast<int64_t>(length))
                {
                    received = true;
                    break;
                }
            }
            if (!received) return ReadFailed;
            feed();
            if (write(buffer, length) != length) return WriteFailed;
            offset += static_cast<uint32_t>(length);
        }
        return Complete;
    }
}
#endif
