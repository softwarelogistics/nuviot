#include "SIMModem.h"
#include "Utils.h"
#include <Update.h>

SIMModem::SIMModem(Display *display, Channel *channel, Console *console, Hal *hal)
{
    m_hal = hal;
    m_channel = channel;
    m_console = console;
    m_display = display;
    m_gpsData = new GPSData();
}

SIMModem::SIMModem(Channel *channel, Console *console, Hal *hal)
{
    m_hal = hal;
    m_channel = channel;
    m_console = console;
    m_display = NULL;
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

void SIMModem::httpCloseSession(String tag)
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
            done = true;
        }
    }

    m_isHttpSessionActive = false;
}

#define BLOCK_SIZE 2 * 1024

uint32_t SIMModem::downloadContent(uint32_t contentSize, unsigned char *buffer)
{
    uint32_t totalBytesRead = 0;

    long start = millis();

    int loopCount = 1;
    while (contentSize > 0 && loopCount < 500)
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

bool SIMModem::httpGetSetError(String url, String errorMsg)
{
    errorMsg.replace(" ", "_");
    errorMsg.replace(".", "_");
    errorMsg.replace(",", "_");
    httpGetNoContent(url + "/failed?err=" + errorMsg);

    return httpGetNoContent(url + "/failed?err=" + errorMsg);
}

#define MAX_CHUNK_DOWNLOAD_TRIES 5

bool SIMModem::beginDownload(String url)
{
    m_display->drawStr("Starting firware", "update process.");

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
        httpCloseSession("fail getting size");
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
                httpCloseSession("invalid content size");
            }
            else
            {
                long bytesDownloaded = downloadContent(contentSize, m_rxBuffer);                

                if (bytesDownloaded != contentSize)
                {
                    m_console->printWarning("fwupdate=downloadchunk; // failed to download chunk, attempt " + String(downloadRetryCount) + " of " + String(MAX_CHUNK_DOWNLOAD_TRIES) + ", may retry");
                    httpCloseSession("failed download chunk.");
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

        int written = Update.write(m_rxBuffer, contentSize);
        if (written < contentSize)
        {
            m_console->printError("fwupdate=failed; // byte array to ota buffer " + String(written) + "/" + String(contentSize) + " - " + String(Update.errorString()) + ".");
            httpGetSetError(url, String(Update.errorString()));
            m_hal->restart(2000);
        }
        else
        {
            m_console->println("fwupdate=writeota; // write " + String(written) + ", block " + String(chunkIndex) + " out of " + String(chunks) + ".");
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
        if (Update.end())
        {
            m_display->drawStr("Success flashing", "Rebooting in 2 seconds.");
            m_console->println("fwupdate=success; // rebooting");
            httpGetNoContent(url + "/success");
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
            m_console->printWarning(S_TIMEOUT);
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
                    m_console->printVerbose(String(m_cmdIdx) + " [" + msg + "] - ok");
                    return S_ERROR;
                }
                else if (msg == cmd)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Returned original -> " + msg);
                }
                else if (msg == S_SMS_READY)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Returned -> " + msg);
                    if (waitForReply(S_CALL_READY, 5))
                    {
                        return S_RESET;
                    }
                }
                else if (msg == S_CALL_READY)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Returned -> " + msg);

                    if (waitForReply(S_SMS_READY, 5))
                    {
                        return S_RESET;
                    }
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
    return connected;

    m_console->printWarning("Checking for modem online");

    int idx = 0;
    while (!connected && idx < 7)
    {
        connected = sendCommand("AT", S_OK, 0, 500, false) == S_OK;

        if (!connected)
        {
            m_console->printWarning("Could not connect changing baud rate: " + String(baudRate[idx]));
            delay(500);

            m_channel->setBaudRate(baudRate[idx]);
            idx++;
        }
    }

    m_console->printWarning("Out of loop " + String(connected));

    return connected;
}

bool SIMModem::selectNetwork()
{
    /* Connect to AT&T network */
    return sendCommand("AT+COPS=4,2,\"310410\"", S_OK, 0, 500, false) == S_OK;
}

bool SIMModem::setNBIoTMode()
{
    if (sendCommand("AT+CMNB=1", S_OK, 0, 500, false) == S_OK)
        return true;

    m_lastError = "COMMS015";
    return false;
}

bool SIMModem::setLTE()
{
    if (sendCommand("AT+CNMP=38", S_OK, 0, 500, false) == S_OK)
        return true;

    m_lastError = "COMMS014";

    return false;
}

bool SIMModem::resetModem()
{
    return sendCommand("AT+CFUN=1,1", "SMS Ready", 0, 15000, false) == S_OK;
}

bool SIMModem::setPDPContext()
{
    String str = "AT+CGDCONT=1,\"IP\",\"hologram\",\"0.0.0.0\",0,0,0,0";
    return sendCommand(str) == S_OK;
}

bool SIMModem::setBand()
{
    int retry = 0;
    while (retry < 5)
    {
        if (sendCommand("AT+CBAND=\"ALL_MODE\"") == S_OK)
        {
            m_console->setVerboseLogging(false);
            return true;
        }

        m_console->printError("ERROR SET BAND, Retry Count " + String(retry));
        m_console->setVerboseLogging(true);

        delay(1500);

        m_lastError = "COMMS016";
    }

    m_console->printError("ERROR SET BAND, WONT RETRY");

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
        return sendCommand("AT+CSTT=" + desiredAPN) == S_OK;
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

    if (sendCommand("AT+SAPBR=1,1") != S_OK)
    {
        m_console->printError("Could not open Bearer");
        return false;
    }

    if (sendCommand("AT+SAPBR=2,1", S_OK, 0, 1500, false) != S_OK)
    {
        m_console->printError("Could not open Bearer");
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

    if (creg.length() < 10)
        return false;

    return (creg.charAt(10) == '1' || creg.charAt(10) == '5');
}

bool SIMModem::init()
{
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
        m_console->printError("connectserver=fail; // CLOSE OK unepected response - " + response);
        return false;
    }

    String connect = "AT+CIPSTART=\"TCP\",\"" + hostName + "\",\"" + port + "\"";

    response = sendCommand(connect, S_CONNECT_OK, 0, 5000, false);

    if (response != S_OK)
    {
        m_console->printError("connectserver=fail; // CONNECT OK unepected response - " + response);
        return false;
    }

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
        m_console->printError("Could not power up GPS");
        return false;
    }
    else
    {
        m_console->printVerbose("Started up GPS");
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

    while (!m_channel->available())
        ;
    m_channel->readByte();
    while (!m_channel->available())
        ;
    m_channel->readByte();

    String gpsData = m_channel->readStringUntil('\r', 100);

    while (!m_channel->available())
        ;
    m_channel->readByte();
    while (!m_channel->available())
        ;
    m_channel->readByte();
    while (!m_channel->available())
        ;
    m_channel->readByte();

    String ok = m_channel->readStringUntil('\r', 100);
    while (!m_channel->available())
        ;
    m_channel->readByte();

    if (ok == "OK")
    {
        m_gpsData->parse(gpsData + "\r");
        return m_gpsData;
    }
    else
    {
        return NULL;
    }
}

bool SIMModem::connect(String apn, String apnUid, String apnPwd)
{
    if (!init())
    {
        return false;
    }

    long start = millis();

    m_apn = apn;
    m_apnUid = apnUid;
    m_apnPwd = apnPwd;

    boolean connected = false;
    while (!connected)
    {
        m_simId = getSIMId();

        start = millis();
        m_console->printVerbose("Connect to cell service.");
        while (!getCGREG())
        {
            m_console->printWarning("Not connect to cell service.");
            delay(1500);

            if (millis() - start > 60000)
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

        start = millis();

        while (!isServiceConnected())
        {
            delay(500);
            m_console->printWarning("Service status is not up, retrying.");

            if (millis() - start > 60000)
            {
                m_lastError = "SIM0011";

                m_console->printError("Timeout waiting for service connection.");
                return false;
            }
        }

        m_console->println("servicestatus=up;");

        m_ipAddress = "";
        if (m_ipAddress == "")
        {
            bool isGPRSConnected = false;
            start = millis();
            while (!isGPRSConnected)
            {
                m_console->printVerbose("GPRS Not Connected - Connecting.");
                delay(500);

                isGPRSConnected = connectGPRS();

                if (millis() - start > 60000)
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