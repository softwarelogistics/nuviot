#include "SIMModem.h"
#include "Utils.h"
#include <Update.h>
#include <esp_spi_flash.h>
#include <esp_ota_ops.h>
#include <esp_image_format.h>

SIMModem::SIMModem(Display *display, Channel *channel, Console *console, Hal *hal, ConfigPins *configPins)
{
    m_hal = hal;
    m_channel = channel;
    m_console = console;
    m_display = display;
    m_configPins = configPins;
    m_gpsData = new GPSData();
}

SIMModem::SIMModem(Channel *channel, Console *console, Hal *hal, ConfigPins *configPins)
{
    m_hal = hal;
    m_channel = channel;
    m_console = console;
    m_display = NULL;
    m_configPins = configPins;
    m_gpsData = new GPSData();
}

bool SIMModem::waitForReply(String expectedReply, int iterations)
{
    bool shouldContinue = true;
    int loopCount = 0;
    while (shouldContinue)
    {
        int bytesToRead = m_channel->available();
        if (bytesToRead >= expectedReply.length())
        {
            m_channel->readBytes(m_tempBuffer, expectedReply.length());

            m_console->printByteArray("Original Message: ", m_tempBuffer, expectedReply.length());

            if (strncmp(expectedReply.c_str(), (char *)m_tempBuffer, expectedReply.length()) == 0)
            {
                m_console->printVerbose("WaitForReply - Match: [" + expectedReply + "] Iteration: " + String(loopCount));
                return true;
            }
        }

        loopCount++;
        if (iterations != -1 && loopCount > iterations)
        {
            m_console->printError("Timed out waiting for: [" + expectedReply + "] Iterations => " + String(iterations));
            return false;
        }

        delay(250);
    }

    return false;
}

bool SIMModem::enableTransparentMode()
{
    return sendCommand("AT+CIPMODE=1") == S_OK;
}

bool SIMModem::disableTransparentMode()
{
    return sendCommand("AT+CIPMODE=0") == S_OK;
}

bool SIMModem::exitDataMode()
{
    delay(1000);
    m_channel->print("+++");
    delay(1000);

    bool result = waitForReply("OK", 5);
    if (result)
    {
        m_console->println("Enable command mode.");
    }
    else
    {
        m_console->printError("Did not enable command mode.");
    }

    return result;
}

uint32_t SIMModem::configureForDownload(String url)
{
    int retryCount = 0;
    String response;
    bool done = false;
    if (!m_isHttpSessionActive)
    {
        while (retryCount++ < 5 && done == false)
        {
            response = sendCommand("AT+HTTPINIT");
            if (response != S_OK)
            {
                m_console->printWarning("fwupdateconfig=unpexpectedresponse; // " + String(retryCount) + " failed http init err: " + response);
                delay(1000);
                m_channel->clearBuffers();
            }
            else
            {
                done = true;
                m_isHttpSessionActive = true;
            }
        }

        if (!done)
        {
            m_console->printError("fwupdateconfig=failed; // could not init http service, err: " + response);
        }

        response = sendCommand("AT+HTTPPARA=\"CID\",1");
        if (response != S_OK)
        {
            m_console->printError("fwupdateconfig=failed; // failed set http param, cid=1, err: " + response);
            return -1;
        }
    }

    String downloadCommand = "AT+HTTPPARA=\"URL\",\"" + url + "\"";

    response = sendCommand(downloadCommand);
    if (response != S_OK)
    {
        m_console->printError("fwupdateconfig=failed; // could not set url " + url + " err: " + response);
        return -1;
    }

    response = sendCommand("AT+HTTPACTION=0");
    if (response != S_OK)
    {
        m_console->printError("fwupdateconfig=failed; // could not set http action, err: " + response);
        return -1;
    }

    retryCount = 0;
    done = false;

    uint32_t contentSize = 0;
    while (!done && retryCount++ < 5)
    {
        String msg = m_channel->readStringUntil('\n', 5000);
        msg.trim();
        if (msg != "")
        {
            contentSize = atol(msg.substring(msg.lastIndexOf(",") + 1).c_str());
            done = true;
        }

        delay(retryCount * 500);
    }

    if (!done)
    {
        m_console->printError("fwupdateconfig=failed; // could not begin download");
        return -1;
    }

    m_channel->transmit("AT+HTTPREAD\r\n");
    String echoResponse = m_channel->readStringUntil('\n', 10000);
    echoResponse.trim();

    String readResponse = m_channel->readStringUntil('\n', 10000);
    readResponse.trim();

    m_console->println("fwupdateconfig=succuss; // download size " + String(contentSize));

    return contentSize;
}

void SIMModem::closeHttpContext(String tag)
{
    int retryCount = 0;
    bool done = false;
    while (retryCount++ < 5 && done == false)
    {
        String response = sendCommand("AT+HTTPTERM");
        if (response != S_OK)
        {
            m_console->printWarning("fwupdate=nohttpterm; // Unexpected response [" + response + "] - area: " + tag + ".");
            delay(500);
            m_channel->clearBuffers();
        }
        else
        {
            String echoResponse = m_channel->readStringUntil('\n', 5000);
            echoResponse.trim();
            done = true;
        }
    }

    m_isHttpSessionActive = false;
}

bool SIMModem::setupHttpContext(String tag, String url)
{
    m_console->println("http" + tag + "=start; // URL: " + url);

    int retryCount = 0;
    String response;
    bool done = false;
    while (retryCount++ < 5 && done == false)
    {
        response = sendCommand("AT+HTTPINIT");
        if (response != S_OK)
        {
            m_console->printWarning("http" + tag + "=unpexpectedresponse; // " + String(retryCount) + " failed http init err: " + response);
            delay(1000);
            m_channel->clearBuffers();
            return false;
        }
        else
        {
            done = true;
            m_isHttpSessionActive = true;
        }
    }

    if (!done)
    {
        m_console->printError("http" + tag + "=failed; // could not init http service, err: " + response);
    }

    response = sendCommand("AT+HTTPPARA=\"CID\",1");
    if (response != S_OK)
    {
        m_console->printError("http" + tag + "=failed; // failed set http param, cid=1, err: " + response);
        return "";
    }

    String httpParam = "AT+HTTPPARA=\"URL\",\"" + url + "\"";
    response = sendCommand(httpParam);
    if (response != S_OK)
    {
        m_console->printError("http" + tag + "=failed; // could not set url " + url + " err: " + response);
        return false;
    }

    return true;
}

String SIMModem::readHttpResponse(String tag)
{
    int retryCount = 0;
    bool done = false;

    String msg = "";
    while (!done && retryCount++ < 5)
    {
        msg = m_channel->readStringUntil('\n', 5000);
        msg.trim();

        if (msg != "")
        {
            if (msg.startsWith("+HTTPACTION:"))
            {
                int first = msg.indexOf(" ");
                int second = msg.indexOf(",", first);
                int third = msg.indexOf(",", second + 1);

                String actionType = msg.substring(first + 1, second);
                String httpResponseCode = msg.substring(second + 1, third);
                if (!httpResponseCode.startsWith("2"))
                {
                    m_console->printError("http" + tag + "=failed; // Non-Success HTTP Response Code: " + httpResponseCode);
                }
                String msgLen = msg.substring(third + 1);
                m_console->printVerbose(msg + " - Action Type=" + actionType + ", HTTP Result Code: " + httpResponseCode + ", Response Length: " + msgLen + ";");
                done = true;
            }
        }

        delay(retryCount * 500);
    }

    if (!done)
    {
        m_console->printError("http" + tag + "=failed; // could not begin download");
        return "";
    }

    m_channel->transmit("AT+HTTPREAD\r\n");
    String echoResponse = m_channel->readStringUntil('\n', 10000);
    echoResponse.trim();

    String readResponse = m_channel->readStringUntil('\n', 10000);
    readResponse.trim();

    String lenStr = readResponse.substring(11);

    memset(m_rxBuffer, 0, RX_BUFFER_SIZE);

    long contentLen = atol(lenStr.c_str());
    m_channel->readBytes(m_rxBuffer, contentLen);

    String content = String((char *)m_rxBuffer);
    m_console->println("http" + tag + "=succuss; // Length: " + lenStr + ", Content Response: " + content);

    m_channel->readStringUntil('\n', 5000);
    String finalOK = m_channel->readStringUntil('\n', 5000);
    return readResponse;
}

String SIMModem::httpGet(String url)
{
    if (!setupHttpContext("get", url))
    {
        m_console->printError("httpget=failed; // could not setup http context.");
        return "";
    }

    String response = sendCommand("AT+HTTPACTION=0");
    if (response != S_OK)
    {
        m_console->printError("httpget=failed; // could not set http action, err: " + response);
        return "";
    }

    response = readHttpResponse("get");

    closeHttpContext("get");

    return response;
}

String SIMModem::httpPost(String url, String payload)
{
    if (!setupHttpContext("post", url))
    {
        m_console->printError("httppost=failed; // could not setup http context.");
        return "";
    }

    String response = sendCommand("AT+HTTPDATA=" + String(payload.length()) + ",10000", "DOWNLOAD", 0, 500, false);
    if (response != S_OK)
    {
        m_console->printError("httppost=failed; // could set http data, err: " + response);
        return "";
    }

    delay(1000);

    m_channel->enqueueString(payload);
    m_channel->flush();

    if (!waitForReply("OK", 100))
    {
        m_console->printError("httppost=failed; // error waiting for OK after post upload");
        return "";
    }

    response = sendCommand("AT+HTTPACTION=1");
    if (response != S_OK)
    {
        m_console->printError("httppost=failed; // could not set http action, err: " + response);
        return "";
    }

    m_console->println("httppost=uploadedpayload; // Length: " + String(payload.length()) + ", Payload: " + payload);

    response = readHttpResponse("post");

    closeHttpContext("post");

    return response;
}

#define BLOCK_SIZE 2 * 1024

uint32_t SIMModem::downloadContent(uint32_t contentSize, unsigned char *buffer)
{
    uint32_t totalBytesRead = 0;

    long start = millis();

    int loopCount = 1;
    while (contentSize > 0 && loopCount < 100)
    {
        long toRead = BLOCK_SIZE < contentSize ? BLOCK_SIZE : contentSize;

        int actualRead = m_channel->readBytes(m_tempBuffer, toRead);

        if (actualRead != -1)
        {
            for (int idx = 0; idx < actualRead; ++idx)
            {
                buffer[idx + totalBytesRead] = m_tempBuffer[idx];
            }

            contentSize -= actualRead;
            totalBytesRead += actualRead;
        }
        else
        {
            loopCount++;
        }
    }

    if (contentSize > 0)
    {
        m_console->printWarning("fwupdatedownload=failed; // remaining content size " + String(contentSize));
    }
    else
    {
        m_console->println("fwupdatedownload=complete; // read " + String(totalBytesRead) + " bytes, " + String(millis() - start) + "ms, " + String(loopCount) + " iterations.");
    }

    if (loopCount < 500)
    {
        waitForReply("OK", 10);
    }

    m_channel->waitForCRLF();
    return totalBytesRead;
}

bool SIMModem::httpGetNoContent(String url)
{
    m_console->printError("httpgetnocontent=start; // " + url);

    long contentSize = configureForDownload(url);
    if (contentSize == -1)
    {
        m_console->printError("httpgetnocontent=failed; // could not configure download");
        return false;
    }

    String response = sendCommand("AT+HTTPTERM");
    if (response != S_OK)
    {
        m_console->printError("httpgetnocontent=failed; // could not terminate http session " + response);
        return false;
    }

    m_console->println("httpgetnocontent=success;");
    return true;
}

String SIMModem::httpGetSetError(String url, String errorMsg)
{
    errorMsg.replace(" ", "_");
    errorMsg.replace(".", "_");
    errorMsg.replace(",", "_");

    return httpGet(url + "/failed?err=" + errorMsg);
}

#define MAX_CHUNK_DOWNLOAD_TRIES 5

bool SIMModem::DisconnectMQTT()
{
    uint8_t disconnectMessage[] = {
        0xE0,
        0x00};

    m_channel->enqueueByteArray(disconnectMessage, 2);

    m_console->printVerbose("NOT TRANSPARENT MODE - CIPSEND");

    uint16_t enqueuedBytes = m_channel->getEnqueuedLength();
    String sendMessage = "AT+CIPSEND=" + String(enqueuedBytes);
    m_channel->println(sendMessage);

    String response = m_channel->readStringUntil('\n', 3000);

    m_console->printVerbose("AT+CIPSEND= response: " + response);

    uint8_t ch = 0x00;
    uint16_t retryCount = 0;
    while (ch != '>' && retryCount++ < 500)
    {
        while (m_channel->available() > 0 && ch != '>')
        {
            ch = m_channel->readByte();
            m_console->printVerbose("UNEXPECTED RESPONSE: [" + String(ch) + "]");
        }
        delay(1);
    }

    if (ch != '>')
    {
        m_console->printError("mqttflush=fail; //timeout waiting for > ");
        return false;
    }

    m_console->printVerbose("RECEIVED: [" + String(ch) + "] Will continue");

    if (!m_channel->flush())
    {
        m_console->printError("mqttdisconnect=false;");
        return false;
    }
    else
    {
        m_console->println("mqttdisconnect=true;");
        delay(1000);
        return true;
    }
}

bool SIMModem::beginDownload(String url)
{
    // send a command to the MQTT server to say...graceful shutdown
    DisconnectMQTT();

    // disconnect the socket from the MQTT server.
    disconnectServer();
    delay(1000);

    m_channel->clearBuffers();
    delay(1000);

    m_display->drawStr("Starting firmware", "update process.");

    m_console->println("fwupdate=start; // url=" + url);

    long start = millis();

    // make sure we are connected to the GPRS service.
    while (!isServiceConnected())
    {
        delay(500);
        m_console->printWarning("servicestatus=notup,willretry;");

        if (millis() - start > 60000)
        {
            m_lastError = "SIM0011";

            m_console->printError("connecttoservice=failed; // will not retry.");
            m_hal->restart();
        }
    }

    if (!setBearer())
    {
        m_console->printError("setbearer=failed;");
        m_hal->restart();
    }

    // get the content size for the body of the request to get this size,
    // this is the actual response size, not size of the content.
    long contentSize = configureForDownload(url + "/size");
    if (contentSize == -1)
    {
        httpGetSetError(url, "fail getting size.");
        closeHttpContext("fail getting size");
        m_hal->restart();
    }

    m_console->println("fwupdate=size; // download size of " + String(contentSize) + ".");

    // download the content as specified in the body above.
    long bytesread = downloadContent(contentSize, m_rxBuffer);

    // make sure the body size matches the size of the content (usually this is just ascii for the size
    // of the entire payload, like 1423500 bytes).
    if (bytesread != contentSize)
    {
        // the body did not match, the header claim for the body.  Abort and restart.
        m_console->printError("fwupdate=failed; // mismatch in bytes read [" + String(bytesread) + "] and content size [" + String(contentSize) + "]");
        httpGetSetError(url, "mismatch in bytes read");
        delay(2000);
        m_hal->restart();
    }

    // Null terminate the string.
    m_rxBuffer[contentSize] = 0;

    // parse the full size of the content from the body.
    long fullFileSize = atol((char *)m_rxBuffer);

    // we download the data in 16K chunks.
    int chunks = (fullFileSize / DOWNLOAD_BUFFER_SIZE) + 1;

    m_console->println("fwupdate=begin; // full size=" + String(fullFileSize) + ", block size " + String(chunks) + " chunks.");

    // really should be no reason why we don't start the update process, just make sure.
    if (!Update.begin(fullFileSize, U_FLASH))
    {
        m_console->printError("fwupdate=failed; // could not begin ota session: " + String(Update.errorString()));
        httpGetSetError(url, String(Update.errorString()));
        m_hal->restart(2000);
    }

    long totalDownloaded = 0;

    for (int chunkIndex = 0; chunkIndex < chunks; ++chunkIndex)
    {
        // identify the beginning of the chunk to ask for from the server.
        long start = chunkIndex * DOWNLOAD_BUFFER_SIZE;

        // if we can't download the reset in one chunk...
        int downloadChunkSize = (fullFileSize - start);
        if (downloadChunkSize > DOWNLOAD_BUFFER_SIZE)
            // ... set it as the download buffer size (not using ternary for clarity)
            downloadChunkSize = DOWNLOAD_BUFFER_SIZE;

        // couple of state variables for retry getting the chunk.
        bool chunkDownloaded = false;
        int downloadRetryCount = 0;

        while (!chunkDownloaded && downloadRetryCount++ < MAX_CHUNK_DOWNLOAD_TRIES)
        {
            m_channel->clearBuffers();
            // start a timer for each chunk.
            long startMS = millis();
            String downloadQueryString = "?start=" + String(start) + "&length=" + String(downloadChunkSize);
            m_console->println("fwupdate=block; // uri= " + downloadQueryString + " chunk " + String(chunkIndex + 1) + " of " + String(chunks) + " chunks in file size of " + fullFileSize + ".");
            m_display->drawStr("Downloading firmware", String("Total: " + String(fullFileSize) + " bytes").c_str(), String("Part " + String(chunkIndex) + " of " + String(chunks)).c_str());

            contentSize = configureForDownload(url + downloadQueryString);
            // will return the size of the block from the header, should match how much we are requesting.
            if (contentSize != downloadChunkSize)
            {
                // we asked for XXXXX bytes, but the header response said content was NOT XXXXX bytes, close the session, and retry if applicable.
                m_console->printWarning("fwupdate=blocksize // expected: " + String(downloadChunkSize) + " returned: " + String(contentSize) + ", attempt " + String(downloadRetryCount) + " of " + String(MAX_CHUNK_DOWNLOAD_TRIES) + ", may retry");
                closeHttpContext("invalid content size");
            }
            else
            {
                long bytesDownloaded = downloadContent(contentSize, m_rxBuffer);

                if (bytesDownloaded != contentSize)
                {
                    m_console->printWarning("fwupdate=downloadchunk; // failed to download chunk, attempt " + String(downloadRetryCount) + " of " + String(MAX_CHUNK_DOWNLOAD_TRIES) + ", may retry");
                    closeHttpContext("failed download chunk.");
                }
                else
                {
                    chunkDownloaded = true;
                    m_console->println("fwupdate=downloaded; // downloaded: " + String(contentSize) + " in: " + String(millis() - startMS) + "ms");
                }
            }
        }

        if (!chunkDownloaded)
        {
            m_console->printError("fwupdate=failed; // could not download chunk.");
            httpGetSetError(url, "could not download chunk");
            m_hal->restart(2000);
        }

        totalDownloaded += contentSize;

        int written = Update.write(m_rxBuffer, contentSize);
        if (written < contentSize)
        {
            m_console->printError("fwupdate=failed; // byte array to ota buffer " + String(written) + "/" + String(contentSize) + " - " + String(Update.errorString()) + ".");
            httpGetSetError(url, String(Update.errorString()));
            m_hal->restart(2000);
        }
        else
        {
            m_console->println("fwupdate=writeota; // write " + String(written) + ", block " + String(chunkIndex + 1) + " out of " + String(chunks) + " bytes: " + String(totalDownloaded) + " of " + String(fullFileSize));
        }

        m_console->println("-");
    }

    if (!Update.isFinished())
    {
        m_console->printError("fwupdate=failed; // download complete, ota not finished.");
        httpGetSetError(url, "reached end not finished");
        m_hal->restart(2000);
    }
    else
    {
        //const esp_partition_t *_partition = esp_ota_get_next_update_partition(NULL);
        //esp_err_t result = esp_ota_set_boot_partition(_partition);

        //m_console->println("setbootpartion=done; // Value: " + String(result));
        delay(1000);

        if (Update.end())
        {
            m_display->drawStr("Success flashing", "Rebooting in 2 seconds.");
            m_console->println("fwupdate=success; // rebooting");
            httpGet(url + "/success");
            m_hal->restart(2000);
        }
        else
        {

            m_display->drawStr("Error flashing", "Rebooting in 2 seconds.");
            m_console->printError("fwupdate=failed; // could not end OTA process: " + String(Update.errorString()));
            httpGetSetError(url, String(Update.errorString()));
            m_hal->restart(2000);
        }
    }

    return true;
}

void SIMModem::hardwareReset()
{
    m_console->println("modem=starthardwareset; // Start Hardware Reset.");

    if (isModemOnline())
    {
        softwarePowerOff();
    }

    int retryCount = 0;
    while (isModemOnline() && retryCount++ < 5)
    {
        delay(1000);
    }

    if (isModemOnline())
    {
        m_console->printError("modem=swpwrofffail; // Software Power Off Failed, trying Hardware Power Off.");
        hardwarePowerOff();

        while (isModemOnline() && retryCount++ < 5)
        {
            delay(500);
        }

        if (isModemOnline())
        {
            m_console->printError("modem=hwpwrofffail; // Hardware Power Off failed, resetting.");
            delay(1000);
            m_hal->restart();
        }
    }
    else
    {
        delay(2000);
        m_console->println("modem=poweroff; // Modem is powered down.");
        hardwarePowerOn();

        while (!isModemOnline() && retryCount++ < 5)
        {
            delay(500);
        }

        if (!isModemOnline())
        {
            m_console->printError("modem=hwpwronfail; // Hardware Power on failed, resetting.");
            m_hal->restart();
        }
        else
        {
            m_console->println("modem=poweron; // Performed hardware reset of modem.");
        }
    }
}

String SIMModem::getDeviceModel()
{
    return sendCommand("AT+CGMM", "", 0, 1250, true);
}

bool SIMModem::hardwarePowerOn()
{
    if (m_configPins->ModemResetPin != -1)
    {
        m_console->println("modem=poweron; // pin: " + String(m_configPins->ModemResetPin) + " set to low");

        pinMode(m_configPins->ModemResetPin, OUTPUT);
        digitalWrite(m_configPins->ModemResetPin, HIGH);
        delay(1600);
        digitalWrite(m_configPins->ModemResetPin, LOW);
        delay(500);
        digitalWrite(m_configPins->ModemResetPin, HIGH);

        m_console->println("modem=poweron; // pin: " + String(m_configPins->ModemResetPin) + " back high.");
        return true;
    }
    else
    {
        m_console->println("modem=poweron; // Power pin not configured.");
        return false;
    }
}

bool SIMModem::hardwarePowerOff()
{
    if (m_configPins->ModemResetPin != -1)
    {
        m_console->println("modem=poweroff; // pin: " + String(m_configPins->ModemResetPin) + " set to low.");

        pinMode(m_configPins->ModemResetPin, OUTPUT);
        digitalWrite(m_configPins->ModemResetPin, HIGH);
        delay(1600);
        digitalWrite(m_configPins->ModemResetPin, LOW);
        delay(1600);
        digitalWrite(m_configPins->ModemResetPin, HIGH);

        m_console->println("modem=poweroff; // pin: " + String(m_configPins->ModemResetPin) + " back to high.");
        return true;
    }
    else
    {
        m_console->println("modem=poweroff; // Power pin not configured.");
        return false;
    }
}

bool SIMModem::softwarePowerOff()
{
    if (isModemOnline())
    {
        sendCommand("AT+CPOWD=1");
        return true;
    }
    else
    {
        return false;
    }
}

String SIMModem::sendCommand(String cmd)
{
    return sendCommand(cmd, "", 0, 500, false);
}

String SIMModem::sendCommand(String cmd, String expectedReply, unsigned long delayMS, long timeout, boolean returnAny)
{
    int start = millis();
    String returnValue = S_OK;
    m_console->printVerbose("Sending Command ==>" + cmd);
    m_channel->transmit(cmd + "\r\n");

    if (delay > 0)
    {
        delay(delayMS);
    }

    boolean shouldContinue = true;
    while (shouldContinue)
    {
        if (millis() - start > timeout)
        {
            m_console->printWarning("sendcommand=warning; // Command " + cmd);
            return "-1";
        }

        if (m_channel->available() > 0)
        {
            String msg = m_channel->readStringUntil('\n', timeout);
            msg.trim();
            m_console->printVerbose(msg);

            if (msg.length() > 0)
            {
                if (msg == expectedReply)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " [" + msg + "] - ok");
                    m_cmdIdx++;
                    return S_OK;
                }
                else if (msg == S_OK && expectedReply == "")
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Will return value of [" + returnValue + "] - ok");
                    m_cmdIdx++;
                    return returnValue;
                }
                else if (msg == S_ERROR)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " [" + msg + "] - error");
                    return S_ERROR;
                }
                else if (msg == "+CPIN: NOT INSERTED")
                {
                    return S_NOSIM;
                }
                else if (msg == cmd)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Returned original -> " + msg);
                }
                else if (msg == S_RDY)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Received -> " + msg);
                }
                else if (msg == S_CPIN_READY)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Received -> " + msg);
                }
                else if (msg == S_SMS_READY)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Returned -> " + msg);
                    receivedSmsReady = true;
                }
                else if (msg == S_CALL_READY)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Returned -> " + msg);
                    receivedCallReady = true;
                }
                else
                {
                    if (returnAny)
                    {
                        m_console->printVerbose(String(m_cmdIdx) + " Returning result [" + msg + "]");
                        return msg;
                    }
                    else
                    {
                        msg.getBytes(m_tempBuffer, msg.length());
                        if (isString(m_tempBuffer, msg.length()))
                        {
                            returnValue = msg;
                            m_console->printVerbose(String(m_cmdIdx) + " pending return value [" + msg + "]");
                        }
                        else
                        {
                            returnValue = "";
                            m_console->printByteArray("Binary Response - Length: " + String(msg.length()) + " Values: ", m_tempBuffer, msg.length());
                        }
                    }
                }
            }
        }
        else
        {
            delay(1);
        }
    }

    return S_ERROR;
}

String SIMModem::getNetwork()
{
    String response = sendCommand("AT+COPS?");
    if (response.length() > 12)
    {
        return response.substring(12, response.length() - 1);
    }
    else
    {
        return "??";
    }
}

bool SIMModem::enableErrorMessages()
{
    return sendCommand("AT+CMEE=2") == S_OK;
}

bool SIMModem::connectGPRS()
{
    return sendCommand("AT+CIICR", "", 0, 85000, false) == S_OK;
}

bool SIMModem::disconnectGPRS()
{
    return sendCommand("AT+CIPSHUT", S_SHUT_OK, 0, 85000, false) == S_OK;
}

bool SIMModem::isServiceConnected()
{
    String response = sendCommand("AT+CGATT?");
    String responseCode = response.substring(8);
    return responseCode == "1";
}

unsigned long baudRate[7] = {4800, 9600, 19200, 38400, 57600, 115200, 230400};

bool SIMModem::isModemOnline()
{
    bool connected = false;
    connected = sendCommand("AT", S_OK, 0, 500, false) == S_OK;

    if (connected)
    {
        m_console->println("modem=online;");
        return true;
    }
    else
    {
        m_console->printError("modem=offline;");
    }

    return connected;

    int idx = 0;
    while (!connected && idx < 7)
    {
        connected = sendCommand("AT", S_OK, 0, 500, false) == S_OK;

        if (!connected)
        {
            m_console->printWarning("modem=offline; // trying baud: " + String(baudRate[idx]));
            delay(500);

            m_channel->setBaudRate(baudRate[idx]);
            idx++;
        }
    }

    m_console->printWarning("modemonline=" + connected ? "true" : "false");
    return connected;
}

bool SIMModem::selectNetwork()
{
    /* Connect to AT&T network */
    if (sendCommand("AT+COPS=4,2,\"310410\"", S_OK, 0, 500, false) == S_OK)
    {
        m_console->printVerbose("modem=network;");
        return true;
    }
    else
    {
        m_lastError = "COMM018";
        return false;
    }
}

bool SIMModem::setNBIoTMode()
{
    if (sendCommand("AT+CMNB=1", S_OK, 0, 500, false) == S_OK)
    {
        m_console->printVerbose("modem=setnbiot;");
        return true;
    }

    m_lastError = "COMMS015";
    return false;
}

bool SIMModem::setLTE()
{
    if (sendCommand("AT+CNMP=38", S_OK, 0, 500, false) == S_OK)
    {
        m_console->printVerbose("modem=setlte;");
        return true;
    }

    m_lastError = "COMMS014";

    return false;
}

bool SIMModem::resetModem()
{
    String response = sendCommand("AT+CFUN=1,1", S_OK, 0, 15000, false);
    if (response == S_NOSIM)
    {
        bool toggle = false;
        while (true)
        {
            m_display->drawStr("ERROR", "NO SIM INSERTED");
            if (toggle)
                m_display->drawStr("ERROR", "NO SIM INSERTED", "!!!!!");
            else
                m_display->drawStr("ERROR", "NO SIM INSERTED");

            m_console->printError("modemreset=error; // NO SIM INSERTED!");
            toggle = !toggle;
            delay(500);
        }
        return false;
    }
    else if (response == S_OK)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool SIMModem::setPDPContext()
{
    //sendCommand("AT+CGDCONT=?");
    m_console->printVerbose("existingvalue;");
    uint8_t retryCount = 0;
    while (retryCount < 5)
    {
        String str = "AT+CGDCONT=1,\"IP\",\"hologram\",\"0.0.0.0\",0,0,0,0";
        //String str = "AT+CGDCONT=1,\"IP\",,,0,0,0,0";
        if (sendCommand(str) == S_OK)
        {
            m_console->printVerbose("modem=pdpcontext;");
            return true;
        }

        delay(1500);
    }

    m_lastError = "COMMS0019";
    m_console->printError("modem=couldnotsetpdpcontext;");
    return false;
}

bool SIMModem::setBand()
{
    int retry = 0;
    while (retry < 5)
    {
        if (sendCommand("AT+CBAND=\"ALL_MODE\"") == S_OK)
        {
            m_console->println("modem=setband;");
            return true;
        }

        delay(1500);
    }

    m_lastError = "COMMS016";
    m_console->printError("modem=couldnotsetband;");

    return false;
}

bool SIMModem::setAPN()
{
    String desiredAPN;
    if (m_apnUid != NULL && m_apnUid.length() > 0 &&
        m_apnPwd != NULL && m_apnPwd.length() > 0)
    {
        desiredAPN = "\"" + String(m_apn) + "\",\"" + m_apnUid + "\",\"" + m_apnPwd + "\"";
    }
    else
    {
        desiredAPN = "\"" + String(m_apn) + "\"";
    }

    String currentAPN = sendCommand("AT+CSTT?");
    currentAPN = currentAPN.substring(7);

    if (currentAPN != desiredAPN)
    {
        if (sendCommand("AT+CSTT=" + desiredAPN) == S_OK)
        {
            m_console->println("modem=setapn; // " + desiredAPN + ";");
            return true;
        }
        else
        {
            m_lastError = "COMMS017";
            return false;
        }
    }
    else
    {
        m_console->println("modem=apncorrect; // " + currentAPN + ";");
    }

    return true;
}

bool SIMModem::setBearer()
{
    if (sendCommand("AT+SAPBR=3,1,\"APN\",\"" + m_apn + "\"") != S_OK)
    {
        m_console->printError("Could not set SAPBR APN");
        return false;
    }

    if (m_apnUid != NULL && m_apnUid.length() > 0 &&
        m_apnPwd != NULL && m_apnPwd.length() > 0)
    {
        if (sendCommand("AT+SAPBR=3,1,\"USER\",\"" + m_apn + "\"") != S_OK)
        {
            m_console->printError("Could not set SAPBR USER");
            return false;
        }

        if (sendCommand("AT+SAPBR=3,1,\"PWD\",\"" + m_apnUid + "\"") != S_OK)
        {
            m_console->printError("Could not set SAPBR PWD");
            return false;
        }
    }

    String response = sendCommand("AT+SAPBR=1,1");
    if (response != S_OK)
    {
        m_console->printError("openbearer=failed; // Response: " + response);
        return false;
    }

    response = sendCommand("AT+SAPBR=2,1", S_OK, 0, 1500, false);
    if (response != S_OK)
    {
        m_console->printError("querybearer=failed; // Response: " + response);
        return false;
    }

    return true;
}

bool SIMModem::disconnectIP()
{
    return (sendCommand("AT+CIPSHUT", S_SHUT_OK, 0, 5000, false) != S_OK);
}

bool SIMModem::getCGREG()
{
    String creg = sendCommand("AT+CGREG?");
    bool success = creg.length() >= 10;
    if(success) {
        success = (creg.charAt(10) == '1' || creg.charAt(10) == '5');
    }

    if (!success){
        m_console->printError("AT+CREG=failed; // Response: " + creg);
    }

    return success;
}

bool SIMModem::init()
{
   if(m_rxBuffer == NULL)
        m_rxBuffer = (byte*)malloc(DOWNLOAD_BUFFER_SIZE);

    if(m_tempBuffer == NULL)
        m_tempBuffer = (byte*)malloc(TEMP_BUFFER_SIZE);

    m_console->printVerbose("Initialization of SIM Modem Started.");

    if (!setBand())
    {
        m_lastError = "Could not set band.";
        m_console->printError("Could not set band.");
        return false;
    }

    if (!setLTE())
    {
        m_lastError = "Could not set LTE.";
        m_console->printError("Could not set LTE.");
        return false;
    }

    if (!setNBIoTMode())
    {
        m_lastError = "Could not set NBIoT Mode.";
        m_console->printError(m_lastError);
        return false;
    }

    m_simId = getSIMId();

    if (!setPDPContext())
    {
        m_lastError = "Could not set PDP Context.";
        m_console->printError(m_lastError);
        return false;
    }

    m_console->printVerbose("Initialization of SIM Modem Completed.");

    return true;
}

String SIMModem::getSIMId()
{
    m_simId = sendCommand("AT+CCID");
    return m_simId;
}

int SIMModem::getSignalQuality()
{
    return m_rssi;
}

int SIMModem::findRSSI()
{
    String csq = sendCommand("AT+CSQ");
    if (csq.length() > 0)
    {
        csq = csq.substring(6);
        m_rssi = csq.substring(0, csq.charAt(1) == ',' ? 1 : 2).toInt();
        return m_rssi;
    }

    return -1;
}

String SIMModem::getIPAddress()
{
    return m_ipAddress;
}

String SIMModem::parseIPAddress()
{
    String ipAddress = sendCommand("AT+CIFSR", "", 0, 500, true);
    if (ipAddress == "0.0.0.0")
    {
        return "";
    }
    else
    {
        /* yeah, this sorta sux, but it checks that the string is in four
         * segments seperated by .
         * guess it gets the job done.
         */
        int idx1 = ipAddress.indexOf('.');
        if (idx1 > -1)
        {
            int idx2 = ipAddress.indexOf('.', idx1 + 1);
            if (idx2 > -1)
            {
                int idx3 = ipAddress.indexOf('.', idx2 + 1);
                if (idx3 > -1)
                {
                    return ipAddress;
                }
            }
        }
    }

    return "";
}

bool SIMModem::connectServer(String hostName, String port)
{
    String response = sendCommand("AT+CIPSHUT", S_SHUT_OK, 0, 5000, false);

    if (response != S_OK)
    {
        m_console->printError("connectserver=fail; // SHUT OK unexpected response- " + response);
        return false;
    }

    response = sendCommand("AT+CIPCLOSE", S_CLOSE_OK, 0, 5000, false);

    // hack: if the connection is not open, we probably will get an error, that's OK.
    // todo: get CIPSTATUS working, maybe that will clean this up a little.
    if (response != S_OK && response != S_ERROR)
    {
        m_console->printError("connectserver=fail; // CLOSE OK unexpected response - " + response);
        return false;
    }

    String connect = "AT+CIPSTART=\"TCP\",\"" + hostName + "\",\"" + port + "\"";

    response = sendCommand(connect, S_CONNECT_OK, 0, 15000, false);

    if (response != S_OK)
    {
        m_console->printError("connectserver=fail; // CONNECT OK unexpected response - " + response);
        return false;
    }

    return true;
}

bool SIMModem::disconnectServer()
{
    String connect = "AT+CIPCLOSE";
    String response = sendCommand(connect, S_CLOSE_OK, 0, 5000, false);

    if (response != S_OK && response != S_CLOSE_OK)
    {
        m_console->printError("disconnectserver=fail; // Disconnect OK unexpected response - " + response);
        return false;
    }

    m_console->println("disconnectserver=success;");
    return true;
}

bool SIMModem::setBaudRate(unsigned long baudRate)
{
    String baudRateCmd = "AT+IPR=" + String(baudRate);
    return sendCommand(baudRateCmd);
}

bool SIMModem::initGPS()
{
    if (sendCommand("AT+CGNSPWR=1", "OK", 0, 1500, false) != S_OK)
    {
        m_console->printError("gps=couldnotpoweron;");
        return false;
    }
    else
    {
        m_console->println("gps=poweredon;");
    }

    if (sendCommand("AT+CGNSCOLD", "OK", 0, 1500, false) != S_OK)
    {
        m_console->printError("gps=couldnotcoldstart;");
        return false;
    }
    else
    {
        m_console->println("gps=coldstart;");
    }

    return true;
}

void SIMModem::startGPS()
{
    initGPS();
}

void SIMModem::stopGPS()
{
    if (sendCommand("AT+CGNSURC=0", S_OK, 0, 1500, false) != S_OK)
    {
        m_console->printError("Could not open Bearer");
    }
}

GPSData *SIMModem::readGPS()
{
    m_channel->print("AT+CGNSINF\r");
    String echo = m_channel->readStringUntil('\r', 100);
    m_channel->waitForCRLF();

    String gpsData = m_channel->readStringUntil('\r', 100);
    m_channel->waitForLF();
    m_channel->waitForCRLF();

    m_console->println(gpsData);

    String ok = m_channel->readStringUntil('\r', 100);
    m_channel->waitForLF();

    if (ok == "OK")
    {
        m_console->printVerbose("gps=readdata;");
        m_gpsData->parse(gpsData + "\r");
        return m_gpsData;
    }
    else
    {
        m_console->printError("gps=couldnotreaddata;");
        return NULL;
    }
}

bool SIMModem::connect(String apn, String apnUid, String apnPwd)
{
    if (!init())
    {
        return false;
    }

    m_apn = apn;
    m_apnUid = apnUid;
    m_apnPwd = apnPwd;

    boolean connected = false;
    uint8_t retryCount = 0;
    while (!connected)
    {
        m_console->printVerbose("Connect to cell service.");
        while (!getCGREG())
        {
            retryCount++;
            m_console->printWarning("Not connect to cell service.");
            delay(retryCount * 250);

            if (retryCount > 20)
            {
                m_lastError = "SIM0010";
                return false;
            }
        }

        m_network = getNetwork();
        m_console->printVerbose("Connected to network: " + m_network);
        m_console->println("network=" + m_network + ";");

        int signalStrength = findRSSI();
        while (signalStrength < 1)
        {
            signalStrength = findRSSI();
        }

        m_console->printVerbose("Signal strength: " + String(signalStrength));
        m_console->println("signalstrength=" + String(signalStrength) + ";");

        setAPN();

        retryCount = 0;
        while (!isServiceConnected())
        {
            retryCount++;
            delay(retryCount * 250);
            m_console->printWarning("Service status is not up, retrying.");

            if (retryCount > 20)
            {
                m_lastError = "SIM0011";
                m_console->printError("connect=failed; // service not connected.");
                return false;
            }
        }

        m_console->println("servicestatus=up;");
        retryCount = 0;
        m_ipAddress = "";
        if (m_ipAddress == "")
        {
            bool isGPRSConnected = false;
            while (!isGPRSConnected)
            {
                retryCount++;
                delay(retryCount * 100);
                m_console->printVerbose("GPRS Not Connected - Connecting.");
            
                isGPRSConnected = connectGPRS();

                if (retryCount > 20)
                {
                    m_lastError = "SIM0012";
                    m_console->printError("Could not connect to GPRS.");
                    return false;
                }
            }

            m_console->printVerbose("GRPS Now connected.");

            m_ipAddress = parseIPAddress();
            m_console->printVerbose("Now Connected IP Address is " + m_ipAddress);
        }
        else
        {
            m_console->printVerbose("Already Connected IP Address is " + m_ipAddress);
        }

        connected = true;
    }

    return connected;
}