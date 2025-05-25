#include "OtaServices.h"
#include <HTTPClient.h>

#include <Update.h>
#include <esp_spi_flash.h>
#include <esp_ota_ops.h>
#include <esp_image_format.h>

#define BLOCK_SIZE RCV_BUFFER_SIZE

#ifdef LCD_DISPLAY
OtaServices::OtaServices(Display *display, Console *console, NuvIoTState *state, SIMModem *modem, Hal *hal)
{
    m_display = display;
    m_hal = hal;
    m_modem = modem;
    m_console = console;
    m_state = state;
}

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
    m_display->clearBuffer();
    m_display->drawStr("Updating Firmware");
    m_display->sendBuffer();
#endif

    return m_modem->beginDownload(url);
}

bool OtaServices::downloadWithModem()
{
#if LCD_DISPLAY
    m_display->clearBuffer();
    m_display->drawStr("Updating Firmware");
    m_display->sendBuffer();
#endif

    if (m_url == NULL || m_url.length() == 0)
    {
        m_console->println(F("[OtaServices__downloadOverWiFi] - no url set."));

        return false;
    }

    m_state->OTAState = 101;

    return m_modem->beginDownload(m_url);
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

#define ERR_UPDATER_BEGIN_RETURNED_FALSE -0x1
#define ERR_UPDATER_NOT_FINISHED -0x2
#define ERR_UPDATER_END_RETURNED_FALSE -0x3
#define ERR_DOWNLOAD_HTTP_NOT_CONNECTED -0x4
#define ERR_DOWNLOAD_HTTP_COULD_NOT_DOWNLOAD -0x5
#define ERR_DOWNLOAD_NON_HTTP_SUCCESS_CODE -0x6
#define ERR_CONTENT_DOWNLOAD_MISMATCH -0x7
#define ERR_UPDATER_FAILED_WRITE -0x8
#define ERR_DOWNLOAD_SIZE -0x9

void OtaServices::markCompleted(HTTPClient *client, String url, bool success)
{
    client->begin(url + (success ? "/success" : "/failed"));
    int responseCode = client->GET();
    m_console->println(String(F("[OtaServices__markCompleted] http.GET()=")) + String(responseCode) + "; url=" + url);
    client->end();
}

int64_t OtaServices::getFileSize(HTTPClient *client, String url)
{
    client->begin(url);
    int responseCode = client->GET();

    uint8_t retryCount = 0;
    boolean done = false;

    m_console->println(String(F("[OtaServices__GetFileSize] http.GET()=")) + String(responseCode) + "; url=" + url);

    if (responseCode == 200)
    {
        String sizeStr = client->getString();
        m_console->println(String(F("[OtaServices__GetFileSize] getstring")) + sizeStr + ";");
        client->end();

        uint32_t contentSize = atol(sizeStr.substring(sizeStr.lastIndexOf(",") + 1).c_str());
        m_console->println(String(F("[OtaServices__GetFileSize] contentSize=")) + String(contentSize) + ";");
        return contentSize;
    }
    else
    {
        client->end();
        return ERR_DOWNLOAD_NON_HTTP_SUCCESS_CODE;
    }
}

int64_t OtaServices::downloadContent(Stream *stream, size_t contentSize){
    long start = millis();

    int loopCount = 1;
    //int actualRead = stream->readBytes(m_recvBuffer, contentSize);

    size_t totalRead = 0;
    while(totalRead < contentSize && loopCount < 100)
    {
        size_t toRead = (contentSize - totalRead < TEMP_BUFFER_SIZE) ? contentSize - totalRead : TEMP_BUFFER_SIZE;

        size_t actualRead = stream->readBytes(m_tempBuffer, toRead);
       
       // m_console->println(String(F("[OtaServices__downloadContent] read partial; // actualread=")) + String(actualRead) + ", contentSize=" + String(contentSize) + ", " + String(loopCount) + " iterations.");
       // delay(100);
        memcpy(m_recvBuffer + totalRead, m_tempBuffer, actualRead);

        totalRead += actualRead;
        loopCount++;
    }   
     
    if (totalRead != contentSize)
    {
        m_console->printError("fwupdatedownload=failed; // actualread=" + String(totalRead) + ", contentSize=" + String(contentSize));
        return -1;
    }
    else
    {
  //      m_console->println(String(F("fwupdatedownload=complete; // read ")) + String(totalRead) + " bytes, " + String(millis() - start) + "ms, " + String(loopCount) + " iterations.");
    }
    return totalRead;
}

int64_t OtaServices::applyBlock(HTTPClient *client, String url, uint32_t start, size_t blockSize)
{
    long startMs = millis();
    url += "?start=" + String(start) + "&length=" + String(blockSize);

    m_console->println(String(F("[OtaServices__applyBlock] url->")) + String(url));

    uint8_t responseCode = -1;
    int retryCount = 0;
    uint32_t bytesRead;
    while(responseCode != 200 && retryCount++ < 5)
    {        
        client->begin(url);
        responseCode = client->GET();

        if(responseCode == 200)
        {
            if(client->getSize() != blockSize)
            {
                m_console->printError(String(F("[OtaServices__applyBlock] size mismatch; // expected=")) + String(blockSize) + ", actual=" + String(client->getSize()));
                return ERR_CONTENT_DOWNLOAD_MISMATCH;
            }
            
            bytesRead = downloadContent(client->getStreamPtr(), blockSize);
            if(bytesRead < 0)
            {
                responseCode = 0;
            }
        }
        else 
        {
            m_console->println(String(F("[OtaServices__applyBlock] responseCode=")) + String(responseCode) + "; status=warning,retryCount: " + String(retryCount));
        }
        client->end();
    }

    if(responseCode != 200)
    {
        m_console->println("[OtaServices__applyBlock] - too many retries");
        return ERR_DOWNLOAD_HTTP_COULD_NOT_DOWNLOAD;
    }

    uint32_t bytesWritten = Update.write(m_recvBuffer, bytesRead);
    if (bytesWritten != bytesRead)
    {
        m_console->println("[OtaServices__applyBlock]  - size mismatch, failed bytesRead=" + String(bytesRead) + ", written=" + String(bytesWritten));
        return ERR_UPDATER_FAILED_WRITE;
    }

    m_console->println(String(F("[OtaServices__applyBlock] bytesWritten=")) + String(bytesWritten) + ", " + String(millis() - startMs) + "ms;" );

    return bytesWritten;
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
    m_console->enableBTOut(false);
    uint32_t freeHeep = ESP.getFreeHeap();
    m_console->println(String(F("[OtaServices__downloadOverWiFi] before allocate for download=")) + String(freeHeep) + " Largest Block Size: " + String(heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT)));

    size_t blockSize = heap_caps_get_largest_free_block (MALLOC_CAP_DEFAULT) * 0.8;

    if (m_recvBuffer == NULL)
        m_recvBuffer = (byte *)malloc(blockSize);
    
    if (m_recvBuffer == NULL) {
        m_console->printError(String(F("[OtaServices__downloadOverWiFi] Could Not Allocate receive buffer")) + String(freeHeep));
        delay(1000);
        m_hal->restart();
    }

    if(m_tempBuffer == NULL) 
        m_tempBuffer = (byte *)malloc(TEMP_BUFFER_SIZE);

    HTTPClient http;
 
    freeHeep = ESP.getFreeHeap();
    m_console->println("[OtaServices__downloadOverWiFi] after allocate, Block Size:" + String(blockSize) + " Temp Buffer: " + String(TEMP_BUFFER_SIZE) + " for download=" + String(freeHeep));

    uint64_t contentSize = getFileSize(&http, url + "/size");
    if(contentSize < 0) {        
        m_console->printError(String(F("[OtaServices__downloadOverWifi] Could not download size")));
        markCompleted(&http, url, false);
        m_hal->restart();
        return ERR_DOWNLOAD_SIZE;
    }

    m_console->println(String(F("[OtaServices__downloadOverWifi] start; // url=")) + url);
    int downloaded = 0;

    uint16_t blockIndex = 0;

    char progressBar[110];

#if LCD_DISPLAY
    m_display->clearBuffer();
    m_display->drawStr(("Updating Firmware"));
    m_display->sendBuffer();
#endif
    m_console->println(String(F("[OtaServices__downloadOverWifi] download-size=")) + String((uint32_t)contentSize) + ";");
    m_hal->feedHWWatchdog();
    m_hal->enableHWWatchdog(2 * 60);

    bool hasCompleted = false;
    uint8_t lastError = 0;

    if (Update.begin((uint32_t)contentSize, U_FLASH))
    {
        while (!hasCompleted)
        {
            uint32_t start = blockIndex * blockSize;
            uint32_t blockEnd = ((uint32_t)start + (uint32_t)blockSize);
            int32_t extras = blockEnd - (uint32_t)contentSize;

            if (extras > 0)
            {
                uint32_t remainder = blockSize - extras;
                blockSize = remainder;
                hasCompleted = true;
            }

            int32_t result = applyBlock(&http, url, start, blockSize);
            m_hal->feedHWWatchdog();

            if (result < 0)
            {
                // Error Condition
                hasCompleted = true;
                lastError = (uint8_t)result;
                markCompleted(&http, url, false);
            }
            else
            {
                blockIndex++;
                downloaded += result;
            }
        }

        if (!Update.isFinished())
        {
            Update.end();
            lastError = ERR_UPDATER_NOT_FINISHED;
            m_console->printError(String(F("[OtaServices__downloadOverWifi] Update.isFinished=returned-false; // url=")) + url);
            markCompleted(&http, url, false);
        }
    }
    else
    {
        m_console->printError(String(F("[OtaServices__downloadOverWifi] Update.begin=returned-false; // url=")) + url);
#if LCD_DISPLAY
        m_display->drawStr(("Flashing init ... failed!"));
#endif
        lastError = ERR_UPDATER_BEGIN_RETURNED_FALSE;
        markCompleted(&http, url, false);
    }

    m_console->println(String(F("[OtaServices__downloadOverWifi] download=complete;")));

    if (downloaded == contentSize)
    {
        if (Update.end())
        {
#ifdef MQTT_VERBOSE
            m_console->println("Success flashing, pausing and then restarting.");
#endif
#if LCD_DISPLAY
            m_display->drawStr("Success flashing", "Restarting");
#endif
            delay(2000);
            m_console->println("[OtaServices__downloadOverWifi] success-flashing=restarting;");
            markCompleted(&http, url, true);
        }
        else
        {
            lastError = ERR_UPDATER_END_RETURNED_FALSE;
            m_console->printError(String(F("Could not flash file ")) + url);
            m_console->printError(String(F("[OtaServices__downloadOverWifi] failure-flashing; // Update.end() returned false")));

#if LCD_DISPLAY
            m_display->drawStr(("Flashing Failed"), ("MD5 Error"));
#endif
            delay(2000);
            markCompleted(&http, url, false);
        }
    }
    else
    {

        m_console->printError(String(F("[OtaServices__downloadOverWifi] downloadsizemismatch; // downloaded=")) + String(downloaded) + String(F(" total:")) + String((uint32_t)contentSize));

#if LCD_DISPLAY
        m_display->drawStr(("Flashing Failed"), ("Download Error"));
#endif
        lastError = ERR_CONTENT_DOWNLOAD_MISMATCH;
        markCompleted(&http, url, false);
    }

    m_hal->restart();

    return lastError;
}