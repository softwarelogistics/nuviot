#include "SIMModem.h"
#include "Utils.h"

SIMModem::SIMModem(Channel *channel, Console *console)
{
    m_channel = channel;
    m_console = console;
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
                m_console->print("WaitForReply - Match: [" + expectedReply + "] Iteration: " + String(loopCount));
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
}

bool SIMModem::enableTransparentMode()
{
    return sendCommand("AT+CIPMODE=1") == S_OK;
}

bool SIMModem::disableTransparentMode()
{
    return sendCommand("AT+CIPMODE=0") == S_OK;
}

bool SIMModem::exitCommandMode()
{
    delay(1000);
    m_channel->print("+++");
    delay(1000);

    bool result = waitForReply("OK", 5);
    if (result)
    {
        m_console->print("Enable command mode.");
    }
    else
    {
        m_console->printError("Did not enable command mode.");
    }

    return result;
}

bool SIMModem::beginDownload(String url)
{
    long start = millis();

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

    if (!setBearer())
    {
        m_console->printError("COULD NOT SET BEARER");
        return false;
    }

    if (sendCommand("AT+HTTPINIT") != S_OK)
    {
        m_console->printError("COULD NOT SEND HTTPINIT=\"URL\",\"....\"");
        return false;
    }

    if (sendCommand("AT+HTTPPARA=\"URL\",\"" + url + "\"") != S_OK)
    {
        m_console->printError("COULD NOT SEND HTTPPARAM=\"URL\",\"....\"");
        return false;
    }

    if (sendCommand("AT+HTTPPARA=\"CID\",1") != S_OK)
    {
        m_console->printError("COULD NOT SEND HTTPPARAM=\"CIDE\",1");
        return false;
    }

    if (sendCommand("AT+HTTPACTION=0") != S_OK)
    {
        m_console->printError("COULD NOT SEND HTTPACTION=0");
        return false;
    }

    m_console->setVerboseLogging(true);

    int retryCount = 0;
    bool done = false;
    long contentSize = 0;
    while (!done && retryCount++ < 5)
    {
        String msg = m_channel->readStringUntil('\n', 10000);
        msg.trim();
        if (msg != "")
        {
            m_console->print("Http Call Response: " + msg);
            contentSize = atol(msg.substring(msg.lastIndexOf(",") + 1).c_str());
            done = true;
        }

        delay(retryCount * 500);
    }

    if (retryCount == 5)
    {
        m_console->printError("No server response from AT+HTTPACTION=0");
        return false;
    }

    m_console->print("Received content size of [" + String(contentSize) + "].");

    m_channel->transmit("AT+HTTPREAD\r\n");
    String echoResponse = m_channel->readStringUntil('\n', 10000);
    echoResponse.trim();

    m_console->print("Echo Response: " + echoResponse);
    String readResponse = m_channel->readStringUntil('\n', 10000);
    readResponse.trim();
    m_console->print("Read Response: " + readResponse);

    while (contentSize > 0)
    {
        long toRead = 128 < contentSize ? 128 : contentSize;

        int actualRead = m_channel->readBytes(m_tempBuffer, toRead);
        if (actualRead != -1)
        {
            contentSize -= actualRead;
            m_tempBuffer[actualRead - 1] = 0x00;

            //Serial.println("RESPONSE MESSAGE" + String((char *)m_tempBuffer));
        }
        else
        {
            m_console->print("Bytes Read [" + String(actualRead) + "] with total of [" + String(contentSize) + "] bytes.");
        }
    }

    m_console->setVerboseLogging(false);
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

bool SIMModem::isModemOnline()
{
    return sendCommand("AT", S_OK, 0, 500, false) == S_OK;
}

bool SIMModem::setNBIoTMode()
{
    if (sendCommand("AT+CMNB=1", S_OK, 0, 500, false) != S_OK)
        return true;

    m_lastError = "COMMS0005";
    return false;
}

bool SIMModem::setLTE()
{
    if (sendCommand("AT+CNMP=38", S_OK, 0, 500, false) != S_OK)
        return true;

    m_lastError = "COMMS0004";

    return false;
}

bool SIMModem::resetModem()
{
    return sendCommand("AT+CFUN=1,1", "SMS Ready", 0, 15000, false) == S_OK;
}

bool SIMModem::setBand()
{
    return sendCommand("AT+CBAND=\"ALL_MODE\"") == S_OK;
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

String SIMModem::getSIMId()
{
    return sendCommand("AT+CCID");
}

int SIMModem::getSignalQuality()
{
    String csq = sendCommand("AT+CSQ");
    if (csq.length() > 0)
    {
        csq = csq.substring(6);
        return csq.substring(0, csq.charAt(1) == ',' ? 1 : 2).toInt();
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

bool SIMModem::connect(String apn, String apnUid, String apnPwd)
{
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
        m_console->print("Connected to network: " + m_network);

        int signalStrength = getSignalQuality();
        while (signalStrength < 1)
        {
            signalStrength = getSignalQuality();
        }

        m_console->print("Signal strength: " + String(signalStrength));

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

        m_console->print("Service status is up.");

        m_ipAddress = "";
        if (m_ipAddress == "")
        {
            bool isGPRSConnected = false;
            start = millis();
            while (!isGPRSConnected)
            {
                m_console->print("GPRS Not Connected - Connecting.");
                delay(500);

                isGPRSConnected = connectGPRS();

                if (millis() - start > 60000)
                {
                    m_lastError = "SIM0012";
                    m_console->printError("Could not connect to GPRS.");
                    return false;
                }
            }

            m_console->print("GRPS Now connected.");

            m_ipAddress = parseIPAddress();
            m_console->print("Now Connected IP Address is " + m_ipAddress);
        }
        else
        {
            m_console->print("Already Connected IP Address is " + m_ipAddress);
        }

        connected = true;
    }
}
