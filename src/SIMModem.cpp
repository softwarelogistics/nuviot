#include "SIMModem.h"
#include "Utils.h"
#include <Update.h>

SIMModem::SIMModem(Display *display, Channel *channel, Console *console)
{
    m_channel = channel;
    m_console = console;
    m_display = display;
}

SIMModem::SIMModem(Channel *channel, Console *console)
{
    m_channel = channel;
    m_console = console;
    m_display = NULL;
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

#ifdef SIM_VERBOSE
            m_console->printByteArray("Original Message: ", m_tempBuffer, expectedReply.length());
#endif            

            if (strncmp(expectedReply.c_str(), (char *)m_tempBuffer, expectedReply.length()) == 0)
            {
#ifdef SIM_VERBOSE                
                m_console->printVerbose("WaitForReply - Match: [" + expectedReply + "] Iteration: " + String(loopCount));
#endif                
                return true;
            }
        }

        loopCount++;
        if (iterations != -1 && loopCount > iterations)
        {
#ifdef SIM_VERBOSE            
            m_console->printError("Timed out waiting for: [" + expectedReply + "] Iterations => " + String(iterations));
#endif            
            return false;
        }

        delay(250);
    }
}

bool SIMModem::enableTransparentMode()
{
    return sendCommand("AT+CIPMODE=1") == S_OK;
}

bool SIMModem::disableTransparentMode()
{
    return sendCommand("AT+CIPMODE=0") == S_OK;
}

bool SIMModem::exitTransparentMode()
{
    delay(1000);
    m_channel->print("+++");
    delay(1000);

    bool result = waitForReply("OK", 5);
#ifdef SIM_VERBOSE
    if (result)
    {
        m_console->println("Enable command mode.");
    }
    else
    {
        m_console->printError("Did not enable command mode.");
    }
#endif    

    return result;
}

long SIMModem::configureForDownload(String url)
{
    if (sendCommand("AT+HTTPINIT") != S_OK)
    {
#ifdef SIM_VERBOSE        
        m_console->printError("COULD NOT SEND HTTPINIT");
#endif        
        return -1;
    }

    if (sendCommand("AT+HTTPPARA=\"CID\",1") != S_OK)
    {
#ifdef SIM_VERBOSE                
        m_console->printError("COULD NOT SEND HTTPPARAM=\"CID\",1");
#endif        
        return -1;
    }

    String downloadCommand = "AT+HTTPPARA=\"URL\",\"" + url + "\"";

    if (sendCommand(downloadCommand) != S_OK)
    {
        m_console->printError("COULD NOT SEND HTTPPARAM=\"URL\",\"....\"");
        return -1;
    }

    if (sendCommand("AT+HTTPACTION=0") != S_OK)
    {
        m_console->printError("COULD NOT SEND HTTPACTION=0");
        return -1;
    }

    int retryCount = 0;
    bool done = false;
    long contentSize = 0;
    while (!done && retryCount++ < 5)
    {
        String msg = m_channel->readStringUntil('\n', 10000);
        msg.trim();
        if (msg != "")
        {
            contentSize = atol(msg.substring(msg.lastIndexOf(",") + 1).c_str());
            done = true;
        }

        delay(retryCount * 500);
    }

    if (retryCount == 5)
    {   
        m_console->printError("No server response from AT+HTTPACTION=0");
        return -1;
    }

    m_channel->transmit("AT+HTTPREAD\r\n");
    String echoResponse = m_channel->readStringUntil('\n', 10000);
    echoResponse.trim();

    String readResponse = m_channel->readStringUntil('\n', 10000);
    readResponse.trim();

    return contentSize;
}

#define BLOCK_SIZE 2 * 1024

long SIMModem::downloadContent(long contentSize, unsigned char *buffer)
{
    long totalBytesRead = 0;
#ifdef SIM_VERBOSE                
    m_console->print("Expected:");
#endif    


    while (contentSize > 0)
    {
        long toRead = BLOCK_SIZE < contentSize ? BLOCK_SIZE : contentSize;

        int actualRead = m_channel->readBytes(m_tempBuffer, toRead);

        if (actualRead != -1)
        {
#ifdef SIM_VERBOSE                            
            m_console->print(" " + String(contentSize));
#endif            

            for (int idx = 0; idx < actualRead; ++idx)
            {
                buffer[idx + totalBytesRead] = m_tempBuffer[idx];
            }

            contentSize -= actualRead;
            totalBytesRead += actualRead;
        }
        else
        {
            m_console->printError("Expected " + String(contentSize) + ", Actual Read " + String(actualRead));
        }
    }

#ifdef SIM_VERBOSE                
    m_console->println(";");
#endif    

    waitForReply("OK", 10);

    return totalBytesRead;
}

bool SIMModem::beginDownload(String url)
{
    m_display->drawStr("Starting firware", "update process.");

    long start = millis();
    while (!isServiceConnected())
    {
        delay(500);
        m_console->printWarning("Service status is not up, retrying.");

        if (millis() - start > 60000)
        {
            m_lastError = "SIM0011";

            m_console->printError("Timeout waiting for service connection.");
            return -1;
        }
    }

    if (!setBearer())
    {
        m_console->printError("COULD NOT SET BEARER");
        return -1;
    }

    long contentSize = configureForDownload(url + "/size");
    if (contentSize == -1)
    {
        return false;
    }

#ifdef SIM_VERBOSE                
    m_console->println("Received content size of [" + String(contentSize) + "].");
#endif    

    long bytesread = downloadContent(contentSize, m_rxBuffer);

    m_console->printByteArray(m_rxBuffer, 20);

    if (bytesread == contentSize)
    {
        m_rxBuffer[contentSize] = 0;
        long fullFileSize = atol((char *)m_rxBuffer);

        if (sendCommand("AT+HTTPTERM") != S_OK)
        {
            m_console->printError("COULD NOT TERMINATE HTTP SESSION");
            return -1;
        }

        int chunks = (fullFileSize / DOWNLOAD_BUFFER_SIZE) + 1;

#ifdef SIM_VERBOSE                
        m_console->println("String full content size to download [" + String(fullFileSize) + "], broken into " + String(chunks) + " chunks.");
#endif        

        if (Update.begin(fullFileSize, U_FLASH))
        {
            for (int chunkIndex = 0; chunkIndex < chunks; ++chunkIndex)
            {
                long start = chunkIndex * DOWNLOAD_BUFFER_SIZE;

                int downloadChunkSize = (fullFileSize - start);
                if (downloadChunkSize > DOWNLOAD_BUFFER_SIZE)
                    downloadChunkSize = DOWNLOAD_BUFFER_SIZE;

                String downloadQueryString = "?start=" + String(start) + "&length=" + String(downloadChunkSize);
#ifdef SIM_VERBOSE                                
                m_console->println("Query String: " + downloadQueryString + "  " + String(chunkIndex + 1) + " of " + String(chunks) + " chunks in file size of " + fullFileSize + "].");
#endif                
                m_display->drawStr("Downloading firmware", String("Total: " + String(fullFileSize) + " bytes").c_str(), String("Part " + String(chunkIndex) + " of " + String(chunks)).c_str());

                contentSize = configureForDownload(url + downloadQueryString);
                if (contentSize != downloadChunkSize)
                {
                    m_console->printError("Expected file size of " + String(downloadChunkSize) + " returned size " + String(contentSize));
                    delay(2000);
                    ESP.restart();
                }

                m_console->println("Received content size of [" + String(contentSize) + "].");

                long bytesDownloaded = downloadContent(contentSize, m_rxBuffer);

                if (bytesDownloaded != contentSize)
                {
                    m_console->printError("Expected to download" + String(contentSize) + " actual bytes " + String(bytesDownloaded));
                    delay(2000);
                    ESP.restart();
                }

                int written = Update.write(m_rxBuffer, contentSize);
                if (written < contentSize)
                {
                    m_console->printError("Expected to write " + String(contentSize) + " actual bytes " + String(written));
                    delay(2000);
                    ESP.restart();
                }

                if (sendCommand("AT+HTTPTERM") != S_OK)
                {
                    m_console->printError("COULD NOT TERMINATE HTTP SESSION");
                    delay(2000);
                    ESP.restart();
                }
            }
        }
        else
        {
            m_console->printError("Could not begin flash process " + String(Update.errorString()));
            delay(2000);
            ESP.restart();
        }
    }
    else
    {
        m_console->printError("Mismatch in bytes read [" + String(bytesread) + "] and content size [" + String(contentSize) + "]");
        delay(2000);
        ESP.restart();
    }

    if (!Update.isFinished())
    {
        m_console->printError("Update Finish returned false.");
        delay(2000);
        ESP.restart();
    }
    else
    {
        if (Update.end())
        {
            m_display->drawStr("Error flashing", "Rebooting in 2 seconds.");
            m_console->printError("Error flashing: " + String(Update.errorString()));
            ESP.restart();
        }
        else
        {
            m_display->drawStr("Success flashing", "Rebooting in 2 seconds.");
            m_console->println("Success flashing, starting new version.");
            delay(2000);
            ESP.restart();
        }
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
#ifdef SIM_VERBOSE                                    
                    m_console->printVerbose(String(m_cmdIdx) + " [" + msg + "] - ok");
#endif                    
                    m_cmdIdx++;
                    return S_OK;
                }
                else if (msg == S_OK && expectedReply == "")
                {
#ifdef SIM_VERBOSE                                                        
                    m_console->printVerbose(String(m_cmdIdx) + " Will return value of [" + returnValue + "] - ok");
#endif                    
                    m_cmdIdx++;
                    return returnValue;
                }
                else if (msg == S_ERROR)
                {
#ifdef SIM_VERBOSE                                                                            
                    m_console->printVerbose(String(m_cmdIdx) + " [" + msg + "] - ok");
#endif                    
                    return S_ERROR;
                }
                else if (msg == cmd)
                {
#ifdef SIM_VERBOSE                                                                                                
                    m_console->printVerbose(String(m_cmdIdx) + " Returned original -> " + msg);
#endif                    
                }
                else if (msg == S_SMS_READY)
                {
#ifdef SIM_VERBOSE                                                              
                    m_console->printVerbose(String(m_cmdIdx) + " Returned -> " + msg);
#endif                    
                    if (waitForReply(S_CALL_READY, 5))
                    {
                        return S_RESET;
                    }
                }
                else if (msg == S_CALL_READY)
                {
#ifdef SIM_VERBOSE                                                              
                    m_console->printVerbose(String(m_cmdIdx) + " Returned -> " + msg);
#endif                    

                    if (waitForReply(S_SMS_READY, 5))
                    {
                        return S_RESET;
                    }
                }
                else
                {
                    if (returnAny)
                    {
#ifdef SIM_VERBOSE                                                                                      
                        m_console->printVerbose(String(m_cmdIdx) + " Returning result [" + msg + "]");
#endif                        
                        return msg;
                    }
                    else
                    {
                        msg.getBytes(m_tempBuffer, msg.length());
                        if (isString(m_tempBuffer, msg.length()))
                        {
                            returnValue = msg;
#ifdef SIM_VERBOSE                                                                                          
                            m_console->printVerbose(String(m_cmdIdx) + " pending return value [" + msg + "]");
#endif
                        }
                        else
                        {
                            returnValue = "";
#ifdef SIM_VERBOSE                                                                                                                      
                            m_console->printByteArray("Binary Response - Length: " + String(msg.length()) + " Values: ", m_tempBuffer, msg.length());
#endif                            
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
    if (sendCommand("AT+CBAND=\"ALL_MODE\"") == S_OK)
        return true;

    m_lastError = "COMMS016";

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
#ifdef SIM_VERBOSE                                                                                                                          
    m_console->printVerbose("Initialization of SIM Modem Started.");
#endif    

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

#ifdef SIM_VERBOSE                                                                                                                      
    m_console->printVerbose("Initialization of SIM Modem Completed.");
#endif    

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
    if (sendCommand("AT+CIPSHUT", S_SHUT_OK, 0, 5000, false) != S_OK)
    {
        return false;
    }

    String connect = "AT+CIPSTART=\"TCP\",\"" + hostName + "\",\"" + port + "\"";
    if (sendCommand(connect, S_CONNECT, 0, 5000, false) != S_OK)
    {
        return false;
    }

    return true;
}

bool SIMModem::setBaudRate(unsigned long baudRate)
{
    // m_channel->setBaudRate(baudRate);

    String baudRateCmd = "AT+IPR=" + String(baudRate);
    Serial.println(baudRateCmd);
    return sendCommand(baudRateCmd);
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
#ifdef SIM_VERBOSE                                                                                                                              
        m_console->printVerbose("Connect to cell service.");
#endif        
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
#ifdef SIM_VERBOSE                                                                                                                              
        m_console->printVerbose("Connected to network: " + m_network);
#endif        

        int signalStrength = findRSSI();
        while (signalStrength < 1)
        {
            signalStrength = findRSSI();
        }

#ifdef SIM_VERBOSE                                                                                                                              
        m_console->printVerbose("Signal strength: " + String(signalStrength));
#endif        

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

        m_console->println("Service status is up.");

        m_ipAddress = "";
        if (m_ipAddress == "")
        {
            bool isGPRSConnected = false;
            start = millis();
            while (!isGPRSConnected)
            {
#ifdef SIM_VERBOSE                                                                                                                                              
                m_console->printVerbose("GPRS Not Connected - Connecting.");
#endif                
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
#ifdef SIM_VERBOSE                                                                                                                                          
            m_console->printVerbose("Now Connected IP Address is " + m_ipAddress);
#endif            
        }
#ifdef SIM_VERBOSE                                                                                                                                      
        else
        {
            m_console->printVerbose("Already Connected IP Address is " + m_ipAddress);
        }
#endif        

        connected = true;
    }
}
