#include "OtaServices.h"
#include <HTTPClient.h>

#include <Update.h>
#include <esp_spi_flash.h>
#include <esp_ota_ops.h>
#include <esp_image_format.h>

#include "OtaTransfer.h"
#include <stdlib.h>
#include <string.h>

namespace
{
    const size_t WifiBlockSize = 4096;
    const size_t SizeBodyCapacity = 32;

    // HTTPClient handles both Content-Length and chunked size responses. This
    // sink never allocates or accepts bytes outside the supplied buffer.
    class BoundedBody : public Stream
    {
        uint8_t *buffer;
        size_t capacity;
        size_t count = 0;
    public:
        BoundedBody(uint8_t *data, size_t limit) : buffer(data), capacity(limit) {}
        size_t size() const { return count; }
        int available() override { return 0; }
        int read() override { return -1; }
        int peek() override { return -1; }
        void flush() override {}
        size_t write(uint8_t value) override { return write(&value, 1); }
        size_t write(const uint8_t *data, size_t length) override
        {
            if (length > capacity - count) return 0;
            memcpy(buffer + count, data, length);
            count += length;
            return length;
        }
    };
}

#ifdef LCD_DISPLAY
#ifdef CELLULAR_ENABLED
OtaServices::OtaServices(Display *display, Console *console, NuvIoTState *state, SIMModem *modem, Hal *hal)
{
    m_display = display;
    m_hal = hal;
    m_modem = modem;
    m_console = console;
    m_state = state;
}
#endif

OtaServices::OtaServices(Display *display, Console *console, NuvIoTState *state, Hal *hal)
{
    m_display = display;
    m_hal = hal;
    m_console = console;
    m_state = state;
}
#endif

#ifdef CELLULAR_ENABLED
OtaServices::OtaServices(Console *console, NuvIoTState *state, SIMModem *modem, Hal *hal)
{
    m_hal = hal;
    m_modem = modem;
    m_console = console;
    m_state = state;
}
#endif

OtaServices::OtaServices(Console *console, NuvIoTState *state, Hal *hal)
{
    m_hal = hal;
    m_console = console;
    m_state = state;
}


OtaServices::~OtaServices()
{
}

#ifdef CELLULAR_ENABLED
bool OtaServices::downloadWithModem(String url)
{
#ifdef LCD_DISPLAY
    if (m_display)
    {
        m_display->clearBuffer();
        m_display->drawStr("Updating Firmware");
        m_display->sendBuffer();
    }
#endif

    return m_modem && url.length() != 0 && m_modem->beginDownload(url);
}

bool OtaServices::downloadWithModem()
{
#ifdef LCD_DISPLAY
    if (m_display)
    {
        m_display->clearBuffer();
        m_display->drawStr("Updating Firmware");
        m_display->sendBuffer();
    }
#endif

    if (m_url == NULL || m_url.length() == 0)
    {
        m_console->println(F("[OtaServices__downloadOverWiFi] - no url set."));

        return false;
    }

    m_state->OTAState = 101;

    return m_modem && m_modem->beginDownload(m_url);
}
#endif

bool OtaServices::start(String url)
{
    #ifdef CELLULAR_ENABLED
    if (m_modem != NULL)
    {
        return downloadWithModem(url);
    }
    else
    {
        return downloadOverWiFi(url);
    }
    #endif
    
    return downloadOverWiFi(url);   
}

void OtaServices::markCompleted(HTTPClient *client, String url, bool success)
{
    m_hal->feedHWWatchdog();
    int status = -1;
    if (client->begin(url + (success ? "/success" : "/failed")))
        status = client->GET();
    client->end();
    m_console->println(String(F("[OTA] terminal HTTP status=")) + String(status));
}

int64_t OtaServices::getFileSize(HTTPClient *client, String url)
{
    uint8_t body[SizeBodyCapacity + 1] = {};
    BoundedBody sink(body, SizeBodyCapacity);
    int result = -1;
    int declared = -1;
    if (client->begin(url) && client->GET() == 200)
    {
        declared = client->getSize();
        if (declared == -1 || (declared > 0 && declared <= static_cast<int>(SizeBodyCapacity)))
            result = client->writeToStream(&sink);
    }
    client->end();
    uint32_t size = 0;
    if (result <= 0 || (declared >= 0 && result != declared) ||
        static_cast<size_t>(result) != sink.size() ||
        memchr(body, 0, sink.size()) ||
        !OtaTransfer::parseDecimal(reinterpret_cast<const char *>(body), INT32_MAX, size) || !size)
        return -1;
    return size;
}

int64_t OtaServices::fetchBlock(HTTPClient *client, const String &url, uint32_t start, uint8_t *buffer, size_t length)
{
    const String query = url + "?start=" + String(start) + "&length=" + String(length);
    BoundedBody sink(buffer, length);
    int result = -1;
    if (client->begin(query))
    {
        const int status = client->GET();
        if (status == 200 && client->getSize() == static_cast<int>(length))
            result = client->writeToStream(&sink);
    }
    client->end();
    if (result < 0 || static_cast<size_t>(result) != length || sink.size() != length)
        return -1;
    return result;
}

bool OtaServices::downloadOverWiFi()
{
    if (m_url == NULL || m_url.length() == 0)
    {
        m_console->println(F("[OtaServices__downloadOverWiFi] - no url set."));

        return false;
    }

    m_state->OTAState = 101;

    return downloadOverWiFi(m_url);
}

bool OtaServices::downloadOverWiFi(String url)
{
    if (!url.length()) return false;
    m_console->enableBTOut(false);
    m_hal->enableHWWatchdog(120);
    m_hal->feedHWWatchdog();

    HTTPClient http;
    http.setReuse(false);
    http.setConnectTimeout(5000);
    http.setTimeout(5000);
    bool success = false;
    bool begun = false;
    const char *failure = "invalid firmware size or partition";
    uint8_t *buffer = NULL;

#ifdef LCD_DISPLAY
    if (m_display)
    {
        m_display->clearBuffer();
        m_display->drawStr("Updating Firmware");
        m_display->sendBuffer();
    }
#endif

    const int64_t size = getFileSize(&http, url + "/size");
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (size > 0 && partition && static_cast<uint64_t>(size) <= partition->size)
    {
        failure = "receive allocation failed";
        buffer = static_cast<uint8_t *>(malloc(WifiBlockSize));
        if (buffer)
        {
            failure = "Update.begin failed";
            begun = Update.begin(static_cast<uint32_t>(size), U_FLASH);
            if (begun)
            {
                const auto result = OtaTransfer::transfer(static_cast<uint32_t>(size), buffer, WifiBlockSize,
                    [&](uint32_t offset, uint8_t *data, size_t length) {
                        return fetchBlock(&http, url, offset, data, length);
                    },
                    [](uint8_t *data, size_t length) { return Update.write(data, length); },
                    [&]() { m_hal->feedHWWatchdog(); });
                m_hal->feedHWWatchdog();
                failure = result == OtaTransfer::ReadFailed ? "chunk download failed" :
                    result == OtaTransfer::WriteFailed ? "flash write failed" : "flash finalization failed";
                success = result == OtaTransfer::Complete && Update.isFinished() && Update.end();
            }
        }
    }

    if (!success && begun) Update.abort();
    free(buffer);
    if (success) m_console->println("[OTA] flash complete");
    else m_console->printError(String("[OTA] ") + failure);
#ifdef LCD_DISPLAY
    if (m_display) m_display->drawStr(success ? "Success flashing" : "Flashing failed", "Restarting");
#endif
    markCompleted(&http, url, success);
    m_hal->feedHWWatchdog();
    delay(2000);
    // Preserve legacy reboot on either outcome (restores communications/BT).
    m_hal->restart();
    return success;
}
