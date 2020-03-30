#include "SimMQTT.h"

#define S_CALL_READY "Call Ready"
#define S_SMS_READY "SMS Ready"
#define S_RESET "RESET!"
#define S_TIMEOUT "TIMEOUT"
#define S_OK "OK"
#define S_AT "AT"
#define S_ERROR "ERROR"

#define S_SEND_OK "SEND OK"
#define S_SHUT_OK "SHUT OK"
#define S_CONNECT_OK "CONNECT OK"

#define MQTT_CONNECT 0x10
#define GPRS_SEND_PACKET 0x1A
#define MQTT_PUBLISH 0x30
#define MQTT_SUBSCRIBE 0x82

SimMQTT::SimMQTT(HardwareSerial *serial, Display *display)
{
    m_display = display;
    m_serial = serial;
}

void SimMQTT::DebugPrint(boolean trace, String msg)
{
    if (trace)
    {
        Serial.println(msg);
    }
}

bool SimMQTT::HandleByteArray(unsigned char *buffer, unsigned int bufferSize, boolean trace)
{
    return true;
}

bool SimMQTT::IsString(byte buffer[], int len)
{
    for (int idx = 0; idx < len; ++idx)
    {
        if ((buffer[idx] < 32 || buffer[idx] > 127) && buffer[idx] != 0x00)
        {
            return false;
        }
    }

    return true;
}

boolean SimMQTT::Loop(boolean trace)
{
    int existingBufferLen = m_serial->available();

    if (existingBufferLen > 0)
    {
        m_serial->readBytes(m_tempBuffer, existingBufferLen);
        PrintByteArray(trace, "Found in buffer: ", m_tempBuffer, existingBufferLen);
    }
    else
    {
        DebugPrint(trace, "empty buffer.");
    }
}

boolean SimMQTT::WaitForReply(String expectedReply, int iterations, boolean trace)
{
    boolean shouldContinue = true;
    int loopCount = 0;
    while (shouldContinue)
    {
        if (m_serial->available() > 0)
        {
            String msg = m_serial->readStringUntil('\n');
            msg.trim();
            if (msg == expectedReply)
            {
                DebugPrint(trace, "WaitForReply - Match: [" + expectedReply + "] Iteration: " + String(loopCount));
                return true;
            }
            else
            {
                if (msg == S_CALL_READY || msg == S_SMS_READY)
                {
                    return false;
                }

                msg.getBytes(m_tempBuffer, msg.length() + 1);
                if (IsString(m_tempBuffer, msg.length()))
                {
                    DebugPrint(trace, "WaitForReply - Expected: [" + expectedReply + "] Acutal: [" + msg + "] - Iterations: " + String(loopCount));
                }
                else
                {
                    PrintByteArray(trace, "WaitForReply - Binary: " + expectedReply + ", Raw Buffer: ", m_tempBuffer, msg.length());
                }

                byte buff[256];
                msg.getBytes(buff, msg.length() + 1);
                HandleByteArray(buff, msg.length(), trace);
            }
        }

        loopCount++;
        if (iterations != -1 && loopCount > iterations)
        {
            DebugPrint(trace, "Timed out waiting for: [" + expectedReply + "]");
            return false;
        }

        delay(250);
    }
}

String SimMQTT::SendCommand(String cmd, String expectedReply, unsigned long delayMS, long timeout, boolean returnAny, boolean trace)
{
    int start = millis();
    String returnValue = S_OK;
    DebugPrint(trace && m_verbose, "==> " + cmd);

    WriteString(cmd + "\r\n", trace);
    Flush();

    if (delay > 0)
    {
        delay(delayMS);
    }

    boolean shouldContinue = true;
    while (shouldContinue)
    {
        if (millis() - start > timeout)
        {
            DebugPrint(trace && m_verbose, S_TIMEOUT);
            return "-1";
        }

        if (m_serial->available() > 0)
        {
            String msg = m_serial->readStringUntil('\r');
            msg.trim();

            if (msg.length() > 0)
            {
                if (msg == expectedReply)
                {
                    DebugPrint(trace && m_verbose, String(m_cmdIdx) + " [" + msg + "] - ok");
                    m_cmdIdx++;
                    return S_OK;
                }
                else if (msg == S_OK && expectedReply == "")
                {
                    DebugPrint(trace && m_verbose, String(m_cmdIdx) + " Will return value of [" + returnValue + "] - ok");
                    m_cmdIdx++;
                    return returnValue;
                }
                else if (msg == S_ERROR)
                {
                    DebugPrint(trace && m_verbose, String(m_cmdIdx) + " [" + msg + "] - ok");
                    return S_ERROR;
                }
                else if (msg == cmd)
                {
                    DebugPrint(trace && m_verbose, String(m_cmdIdx) + " Returned original -> " + msg);
                }
                else if (msg == S_SMS_READY)
                {
                    DebugPrint(trace && m_verbose, String(m_cmdIdx) + " Returned -> " + msg);
                    if (WaitForReply(S_CALL_READY, 5, true && m_verbose))
                    {
                        return S_RESET;
                    }
                }
                else if (msg == S_CALL_READY)
                {
                    DebugPrint(trace && m_verbose, String(m_cmdIdx) + " Returned -> " + msg);

                    if (WaitForReply(S_SMS_READY, 5, true && m_verbose))
                    {
                        return S_RESET;
                    }
                }
                else
                {
                    if (returnAny)
                    {
                        DebugPrint(trace && m_verbose, String(m_cmdIdx) + " Returning result [" + msg + "]");
                        return msg;
                    }
                    else
                    {
                        msg.getBytes(m_tempBuffer, msg.length());
                        if (IsString(m_tempBuffer, msg.length()))
                        {
                            returnValue = msg;
                            DebugPrint(trace && m_verbose, String(m_cmdIdx) + " pending return value [" + msg + "]");
                        }
                        else
                        {
                            returnValue = "";
                            PrintByteArray(true, "  binary response: ", m_tempBuffer, msg.length());
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

String SimMQTT::SendCommand(String cmd, boolean trace)
{
    return SendCommand(cmd, "", 0, 500, false, trace);
}

void SimMQTT::EnableErrorMessages(boolean trace)
{
    SendCommand("AT+CMEE=2", trace);
}

int SimMQTT::GetSignalQuality(boolean trace)
{
    String csq = SendCommand("AT+CSQ", trace);
    if (csq.length() > 0)
    {
        csq = csq.substring(6);
        return csq.substring(0, csq.charAt(1) == ',' ? 1 : 2).toInt();
    }

    return -1;
}

#define BAND = "ALL_BAND"

void SimMQTT::SetBand(boolean trace)
{
    SendCommand("AT+CBAND?", trace);
    SendCommand("AT+CBAND=\"ALL_MODE\"", trace);
}

boolean SimMQTT::GetCREG(boolean trace)
{
    String creg = SendCommand("AT+CGREG?", trace);

    if (creg.length() < 10)
        return false;

    return (creg.charAt(10) == '1' || creg.charAt(10) == '5');
}

boolean SimMQTT::GetCGREG(boolean trace)
{
    String creg = SendCommand("AT+CGREG?", trace);

    if (creg.length() < 10)
        return false;

    return (creg.charAt(10) == '1' || creg.charAt(10) == '5');
}

boolean SimMQTT::IsSIM800Online(boolean trace)
{
    boolean isOnline = SendCommand("AT", S_OK, 0, 500, false, trace) == S_OK;
    if (isOnline)
    {
        EnableErrorMessages(trace);
        if (SendCommand("AT+CIPSHUT", S_SHUT_OK, 0, 5000, false, trace) != S_OK)
        {
            return false;
        }

        if (SendCommand("AT+CNMP=38", S_OK, 0, 500, false, trace) != S_OK)
        {
            return false;
        }

        if (SendCommand("AT+CMNB=1", S_OK, 0, 500, false, trace) != S_OK)
        {
            return false;
        }
    }
    return isOnline;
}

String SimMQTT::GetSIMId(boolean trace)
{
    return SendCommand("AT+CCID", trace);
}

String SimMQTT::GetNetwork(boolean trace)
{
    String response = SendCommand("AT+COPS?", trace);
    if (response.length() > 12)
    {
        return response.substring(12, response.length() - 1);
    }
    else
    {
        return "??";
    }
}

void SimMQTT::PrintByte(uint8_t ch)
{
    char hexChar[2];
    Serial.print("0x");
    sprintf(hexChar, "%02X", ch);
    Serial.print(hexChar);
}

void SimMQTT::PrintByteArray(boolean trace, byte buffer[], int len)
{
    if (trace)
    {
        Serial.print("Byte Array: " + String(len) + " chars ");

        for (int idx = 0; idx < len; ++idx)
        {
            PrintByte(buffer[idx]);
            if (idx < len - 1)
                Serial.print(" ");
        }

        Serial.println(";");
    }
}

void SimMQTT::PrintByteArray(boolean trace, String prefix, byte buffer[], int len)
{
    if (trace)
    {
        Serial.print(prefix);
        for (int idx = 0; idx < len; ++idx)
        {
            PrintByte(buffer[idx]);
            if (idx < len - 1)
                Serial.print(" ");
        }

        Serial.println(";");
    }
}

void SimMQTT::SetStatusUpdateCallback(void (*callback)(String status))
{
    this->callback = callback;
}

void SimMQTT::SetVerboseLogging(boolean enabled)
{
    m_verbose = enabled;
}

boolean SimMQTT::ConnectGPRS(boolean trace)
{
    return SendCommand("AT+CIICR", "", 0, 85000, false, trace) == S_OK;
}

boolean SimMQTT::IsServiceStarted(boolean trace)
{
    String response = SendCommand("AT+CGATT?", trace);
    String responseCode = response.substring(8);
    return responseCode == "1";
}

String SimMQTT::GetIPAddress(boolean trace)
{
    String ipAddress = SendCommand("AT+CIFSR", "", 0, 500, true, trace);
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

void SimMQTT::EnqueueByte(uint8_t byte)
{
    m_txBuffer[m_txTail] = byte;
    m_txTail++;
    if (m_txTail == TX_BUFFER_SIZE)
    {
        m_txTail = 0;
    }
}

void SimMQTT::EneuqueByteArray(uint8_t buffer[], int len)
{
    for (int idx = 0; idx < len; ++idx)
    {
        EnqueueByte(buffer[idx]);
    }
}

void SimMQTT::Flush()
{
    if (write_it_all)
    {
        Serial.println("Data: " + String(m_txHead) + " " + String(m_txTail));
    }

    if (m_txTail < m_txHead)
    {
        for (int idx = m_txHead; idx < TX_BUFFER_SIZE; ++idx)
        {
            m_serial->write(m_txBuffer[idx]);
        }

        for (int idx = 0; idx < m_txTail; ++idx)
        {
            m_serial->write(m_txBuffer[idx]);
        }
    }
    else
    {
        if (write_it_all)
        {
            PrintByteArray(true, &m_txBuffer[m_txHead], m_txTail - m_txHead);
        }

        for (int idx = m_txHead; idx < m_txTail; ++idx)
        {
            m_serial->write(m_txBuffer[idx]);
        }
    }

    m_txHead = m_txTail;
}

void SimMQTT::SetAPN(boolean trace)
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

    String currentAPN = SendCommand("AT+CSTT?", trace);
    currentAPN = currentAPN.substring(7);

    if (currentAPN != desiredAPN)
    {
        SendCommand("AT+CSTT=" + desiredAPN, trace);
    }
}

void SimMQTT::WriteString(String str, boolean trace)
{
    str.getBytes(m_tempBuffer, str.length() + 1);

    WriteByteArray(m_tempBuffer, str.length(), trace);
}

void SimMQTT::WriteLengthPrefixedString(String string, bool trace)
{
    byte lenBuffer[2];
    lenBuffer[0] = (byte)(string.length() >> 8);
    lenBuffer[1] = (byte)(string.length() & 0xFF);
    WriteByteArray(lenBuffer, 2, trace);
    WriteString(string, trace);

    if (trace)
    {
        Serial.println("Sending [" + String(string.length() + 2) + "] bytes for [" + string + "]");
    }
}

void SimMQTT::WriteByteArray(uint8_t array[], int len, bool trace)
{
    EneuqueByteArray(array, len);
    PrintByteArray(trace, array, len);
}

boolean SimMQTT::ReadByteArray(boolean trace, byte buffer[], int expectedLen)
{
    return ReadByteArray(trace, buffer, expectedLen, 5000);
}

boolean SimMQTT::ReadByteArray(boolean trace, byte buffer[], int expectedLen, long timeOutMS)
{
    long startMS = millis();
    while (m_serial->available() < expectedLen &&
           ((millis() - startMS) < timeOutMS))
    {
        delay(5);
    }

    if (m_serial->available() < expectedLen)
    {
        Serial.println("Not enough response to read, timed out.");
        return false;
    }

    m_serial->readBytes(buffer, expectedLen);
    PrintByteArray(trace, m_tempBuffer, expectedLen);

    return true;
}

bool SimMQTT::ReadMQTTResponse(uint8_t expected, bool trace)
{
    int loopCount = 5;
    while (loopCount-- > 0)
    {
        if (trace)
        {
            Serial.print("Waiting for: ");
            PrintByte(expected);
            Serial.println(" from MQTT Server.");
        }

        if (!ReadByteArray(trace && m_verbose, m_tempBuffer, 1))
        {
            Serial.println("Timed out waiting for response from MQTT.");
            return false;
        }

        if (expected == (byte)(m_tempBuffer[0] & expected))
        {
            DebugPrint(trace, "Received valid MQTT response.");

            if (ReadByteArray(trace && m_verbose, m_tempBuffer, 1))
            {
                int payloadLen = m_tempBuffer[0];
                if (payloadLen > 0)
                {
                    m_serial->readBytes(m_tempBuffer, payloadLen);
                    PrintByteArray(trace, "Reading MQTT Response Payload: ", m_tempBuffer, payloadLen);
                }

                return true;
            }
        }

        Serial.print("Invalid response: Expected byte: ");
        PrintByte(expected);
        Serial.print(" received byte: ");
        PrintByte(m_tempBuffer[0]);
        Serial.println(" from MQTT Server.");

        delay(100);
    }

    return false;
}

int SimMQTT::Subscribe(String topic, bool trace)
{
    Loop(trace && m_verbose);

    callback("Subscribing to: " + topic);
    byte payloadLen = 2 + 2 + topic.length() + 1;

    byte subscribeHeader[] = {
        0x82,
        payloadLen,
        0x00, m_subscriptionId};

    m_subscriptionId++;

    WriteByteArray(subscribeHeader, sizeof(subscribeHeader), trace && m_verbose);
    WriteLengthPrefixedString(topic, trace && m_verbose);

    byte qos[] = {
        0x00,
    };

    WriteByteArray(qos, 1, trace && m_verbose);
    write_it_all = true;
    if (SendBuffer(trace && m_verbose))
    {
        write_it_all = false;

        if (ReadMQTTResponse(0x90, trace && m_verbose))
        {
            DebugPrint(trace, "Subscribed to [" + topic + "]");
            callback("Subscribed to: " + topic);
            /* we previously incremented it so decrement it now */
            return m_subscriptionId - 1;
        }
        else
        {
            DebugPrint(trace, "Could not subscribe to [" + topic + "]");
            return -1;
        }
    }
    else
    {
        DebugPrint(trace, "Timeout waiting for SEND_OK on subscribe [" + topic + "]");
    }
    return -1;
}

void SimMQTT::WriteControlField(byte packetHeader, boolean trace)
{
    if (trace)
    {
        Serial.print("Packet header: [");
        PrintByte(packetHeader);
        Serial.println("];");
    }

    EnqueueByte(packetHeader);
}

void SimMQTT::WriteRealLength(int realLength, boolean trace)
{
    if (trace)
    {
        Serial.println("Real Length: [" + String(realLength) + "] total packet length: [" + String(realLength + 2) + "]");
    }

    EnqueueByte((uint8_t)realLength);
}

boolean SimMQTT::Ping(boolean trace)
{
    Loop(trace && m_verbose);

    byte pingMsg[] = {
        0xC0, 0x00};

    WriteByteArray(pingMsg, sizeof(pingMsg), trace && m_verbose);
    SendBuffer(trace && m_verbose);

    return ReadMQTTResponse(0xD0, trace && m_verbose);
}

boolean SimMQTT::Publish(String topic, String payload, byte qos, boolean trace)
{
    Loop(trace && m_verbose);

    byte rl = 2 + topic.length() +
              2 + // packet id
              payload.length();
    byte controlField = 0x30;

    controlField = controlField | (qos << 1);

    WriteControlField(controlField, trace && m_verbose);
    WriteRealLength(rl, trace && m_verbose);

    byte packetIdBuffer[] = {
        (m_packetId >> 8) && 0xFF,
        m_packetId && 0xFF};

    m_packetId++;

    WriteLengthPrefixedString(topic, trace && m_verbose);

    WriteByteArray(packetIdBuffer, 2, trace && m_verbose);

    Serial.println("[" + payload + "] len: " + String(payload.length()));

    WriteString(payload, trace);

    if (!SendBuffer(trace && m_verbose))
    {
        return false;
    }

    return ReadMQTTResponse(0x40, trace && m_verbose);
}

boolean SimMQTT::Publish(String topic, byte qos, boolean trace)
{
    Loop(trace && m_verbose);

    byte rl = 2 + topic.length() +
              2; // packet id
    byte controlField = 0x30;

    controlField = controlField | (qos << 1);

    WriteControlField(controlField, trace && m_verbose);
    WriteRealLength(rl, trace && m_verbose);

    WriteLengthPrefixedString(topic, trace);

    EnqueueByte((uint8_t)((m_packetId >> 8) && 0xFF));
    EnqueueByte((uint8_t)(m_packetId && 0xFF));

    m_packetId++;

    if (!SendBuffer(trace && m_verbose))
    {
        return false;
    }

    if (qos > QOS0)
    {
        return ReadMQTTResponse(0x40, trace && m_verbose);
    }
    else
    {
        return true;
    }
}

bool SimMQTT::ConnectMQTT(String site, String uid, String pwd, boolean trace)
{
    return ConnectMQTT(site, uid, pwd, "mqttclient", trace);
}

bool SimMQTT::ConnectMQTT(String site, String clientId, boolean trace)
{
    return ConnectMQTT(site, "", "", clientId, trace);
}

bool SimMQTT::ConnectMQTT(String site, boolean trace)
{
    return ConnectMQTT(site, "", "", "mqttclient", trace);
}

/* should be called to make sure the state is reset before doing any work with 
 * the modem.  If we are in a state where it's expecting a buffer to be send
 * this will clear it. */
void SimMQTT::ResetModemBuffer()
{
    m_serial->write(0x1a);
    delay(1000);
    int available = m_serial->available();
    if (available > 0)
    {
        m_serial->readBytes(m_tempBuffer, available);
        PrintByteArray(true, "Found and disposed: ", m_tempBuffer, available);
    }

    m_rxHead = 0;
    m_rxTail = 0;
    m_txHead = 0;
    m_txTail = 0;
}

boolean SimMQTT::SendBuffer(boolean trace)
{
    int sendLength = m_txTail > m_txHead ? (m_txTail - m_txHead) : (TX_BUFFER_SIZE - m_txHead) + m_txTail;
    write_it_all = true;
    String startSend = "AT+CIPSEND=" + String(sendLength);

    Serial.println(startSend);
    m_serial->println(startSend);
    WaitForReply(startSend, 5, trace);

    bool shouldContinue = true;
    while (shouldContinue)
    {
        int recvCount = m_serial->available();
        if (recvCount > 0)
        {
            ReadByteArray(trace, m_tempBuffer, 1);
            if (m_tempBuffer[0] == '>')
            {
                shouldContinue = false;
            }
        }
    }

    Flush();
    write_it_all = false;
    return WaitForReply(S_SEND_OK, 5, trace);
}

bool SimMQTT::ConnectMQTT(String site, String uid, String pwd, String clientId, boolean trace)
{
    callback("Connect to MQTT");

    if (SendCommand("AT+CIPSHUT", S_SHUT_OK, 0, 5000, false, trace) != S_OK)
    {
        return false;
    }

    String connect = "AT+CIPSTART=\"TCP\",\"" + site + "\",\"1883\"";
    if (SendCommand(connect, S_CONNECT_OK, 0, 5000, false, trace) != S_OK)
    {
        return false;
    }

    bool isAuth = uid.length() > 0 && pwd.length() > 0;

    String MQTT_HEADER = "MQTT";

    int realLength = 2 + MQTT_HEADER.length() + // MQTT String Plus length
                     1 + 1 + 2 +                //  0x40, 0x02, 0x00, 0x3C
                     2 + clientId.length();

    byte connectFlag = 0x03;

    if (isAuth)
    {
        realLength += 2 + uid.length() + 2 + pwd.length();
        connectFlag = 0xC2;
    }

    byte mqttHeader[] = {
        0x04,        // Protocol Version
        connectFlag, // Connect Flags
        0x00, 0x3C,  // Keep Alive
    };

    WriteControlField(MQTT_CONNECT, trace && m_verbose);
    WriteRealLength(realLength, trace && m_verbose);

    WriteLengthPrefixedString(MQTT_HEADER, trace && m_verbose);

    WriteByteArray(mqttHeader, sizeof(mqttHeader), trace && m_verbose);

    WriteLengthPrefixedString(clientId, trace && m_verbose);
    if (isAuth)
    {
        WriteLengthPrefixedString(uid, trace && m_verbose);
        WriteLengthPrefixedString(pwd, trace && m_verbose);
    }

    if (!SendBuffer(trace && m_verbose))
    {
        return false;
    }

    if (ReadMQTTResponse(0x20, trace && m_verbose))
    {
        callback("Connected to MQTT");
        DebugPrint(trace, "Connected to MQTT");
        return true;
    }
    else
    {
        callback("Not connect to MQTT");
        DebugPrint(trace, "Not connect to MQTT.");
        return false;
    }
}

bool SimMQTT::Connect(String apn, String apnUid, String apnPwd, boolean trace)
{
    ResetModemBuffer();

    long start = millis();

    m_apn = apn;
    m_apnUid = apnUid;
    m_apnPwd = apnPwd;

    DebugPrint(trace, "Begin SIM800L Startup.");
    callback("Begin Connection");

    boolean connected = false;
    while (!connected)
    {
        DebugPrint(trace, "Starting communication with modem.");

        callback("Start commo w/ modem.");

        while (!IsSIM800Online(trace))
        {
            delay(500);
            DebugPrint(trace, "Pending commo w/ modem.");
            callback("Pending commo w/ modem.");
            if (millis() - start > 60000)
            {
                callback("No commo with modem.");
                return false;
            }
        }

        SetBand(trace);

        GetSIMId(trace);

        callback("Connect to cell service");

        DebugPrint(trace, "Connect to cell service.");
        while (!GetCGREG(trace) )
        {
            DebugPrint(trace, "Not connect to cell service.");
            callback("No connect to cell service");
            delay(1500);

            if (millis() - start > 60000)
            {
                callback("No connect to cell service");
                return false;
            }
        }

        String network = GetNetwork(trace);
        DebugPrint(trace, "Connected to network: " + network);
        callback("Connected to: " + network);

        int signalStrength = GetSignalQuality(trace);
        while (signalStrength < 1)
        {
            signalStrength = GetSignalQuality(trace);
        }

        DebugPrint(trace, "Signal strength: " + String(signalStrength));

        SetAPN(trace);

        while (!IsServiceStarted(trace))
        {
            delay(500);
            DebugPrint(trace, "Service status is not up, retrying.");
            callback("Starting service");

            if (millis() - start > 60000)
            {
                callback("Could not start service.");
                return false;
            }
        }

        callback("Service started");

        DebugPrint(trace, "Service status is up.");

        String ipAddress = ""; // GetIPAddress(trace);
        if (ipAddress == "")
        {
            DebugPrint(trace, "GPRS Not Connected - Connecting.");
            callback("Connecting to GPRS");
            boolean isGPRSConnected = false;
            while (!isGPRSConnected)
            {
                DebugPrint(trace, "Not Connected.");
                callback("GPRS Not Connected");
                delay(500);

                isGPRSConnected = ConnectGPRS(trace);

                if (millis() - start > 60000)
                {
                    callback("Could not connect to GPRS.");
                    return false;
                }
            }

            DebugPrint(trace, "GRPS Now connected.");
            callback("GPRS is connected");

            String ipAddress = GetIPAddress(trace);
            DebugPrint(trace, "Now Connected IP Address is " + ipAddress);
            callback("IP Address: " + ipAddress);
        }
        else
        {
            DebugPrint(trace, "Already Connected IP Address is " + ipAddress);
            callback("IP Address: " + ipAddress);
        }

        connected = true;
    }
}
