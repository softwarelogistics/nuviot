#include "OtaServices.h"
#include <HTTPClient.h>

#include <Update.h>

OtaServices::OtaServices(Display *display, Console *console, SIMModem *modem, Hal *hal)
{
    m_display = display;
    m_hal = hal;
    m_modem = modem;
    m_console = console;
}

OtaServices::OtaServices(Display *display, Console *console, Hal *hal)
{
    m_display = display;
    m_hal = hal;
    m_console = console;
}

OtaServices::~OtaServices()
{
}

bool OtaServices::downloadWithModem(String url)
{
    int downloaded = 0;
    int written = 0;
    int total = 1;
    int len = 1;
    uint8_t buff[1024] = {0};
    size_t size = sizeof(buff);
    int ret = 0;

    char progressBar[110];

    m_display->clearBuffer();
    m_display->drawStr("Updating Firmare");
    m_display->sendBuffer();

    m_modem->beginDownload(url);
}

bool OtaServices::start(String url)
{
    if (m_modem != NULL)
    {
        return downloadWithModem(url);
    }
    else
    {
        return downloadOverWiFi(url);
    }
}

bool OtaServices::downloadOverWiFi(String url)
{
    int downloaded = 0;
    int written = 0;
    int total = 1;
    int len = 1;
    uint8_t buff[1024] = {0};
    size_t size = sizeof(buff);
    int ret = 0;
    HTTPClient http;

    char progressBar[110];

    /*if(info.caCert != NULL){
        http.begin(info.url, info.caCert);
    } else {*/
    http.begin(url);
    //}

    int httpCode = http.GET();

    if (httpCode > 0 && httpCode == HTTP_CODE_OK)
    {
        m_display->clearBuffer();
        m_display->drawStr("Updating Firmare");
        m_display->sendBuffer();

        // get lenght of document (is -1 when Server sends no Content-Length header)
        len = http.getSize();
        total = len;
        downloaded = 0;
        // get tcp stream
        WiFiClient *stream = http.getStreamPtr();
        if (Update.begin(total, U_FLASH))
        {
            // Update.setMD5(info.md5);
            downloaded = 0;
            while (!Update.isFinished())
            {
                // read all data from server
                if (http.connected() && (len > 0))
                {
                    // get available data size
                    size = stream->available();

                    if (size > 0)
                    {
                        // read up to 128 byte
                        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                        // write to storage
                        written = Update.write(buff, c);
                        if (written > 0)
                        {
                            int percentCommplete = (100 * downloaded) / total;
                            const char *str = String("Progress " + String(percentCommplete)).c_str();
                            for (int idx = 0; idx < percentCommplete / 10; ++idx)
                            {
                                progressBar[idx] = '.';
                            }

                            progressBar[percentCommplete] = 0x00;

                            m_display->drawStr("Updating Firmare", str, progressBar);
                            if (written != c)
                            {
                                //m_display->drawString(0,48, "Partial Recieve - likely failure");
                            }

                            downloaded += written;
                        }
                        else
                        {
                            m_display->drawStr("Flashing failed");
                            ret = -1;
                            break;
                        }

                        if (len > 0)
                        {
                            len -= c;
                        }
                    }
                    delay(1);
                }
            }
        }
        else
        {
            m_display->drawStr("Flashing init ... failed!");
            ret = -1;
        }
    }
    else
    {
        m_display->drawStr("HTTP Get Failed");
        ret - 1;
    }

    http.end();
    if (downloaded == total && len == 0)
    {
        if (Update.end())
        {
            m_display->drawStr("Success flashing", "Restarting");
            delay(2000);
            m_hal->restart();
        }
        else
        {
            m_display->drawStr("Flasing Failed", "MD5 Error");
            delay(2000);
            ret = -1;
        }
    }
    else
    {
        m_display->drawStr("Flashing Failed", "Download Error");
        ret = -1;
    }

    return ret;
}