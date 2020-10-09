#include "NuvIoTClient.h"

#include <Arduino.h>

NuvIoTClient *nuviotClient;

/*void SimMQTT_Callback(String msg)
{
    if (state.getVerboseLogging())
    {
        if (m_lastMsg != msg)
        {
            Serial.println(msg);
            m_display->clearBuffer(0xFFFFFF);
            m_display->println(msg);
            m_display->sendBuffer();
        }
    }

    m_lastMsg = msg;
}*/

void messagePublished_CallBack(String topic, unsigned char *payload, size_t length)
{
    nuviotClient->messagePublished(topic, payload, length);
}

NuvIoTClient::NuvIoTClient(SIMModem *modem, MQTT *mqtt, Console *console, Display *display, LedManager *ledManager, NuvIoTState *state, SysConfig *sysConfig, OtaServices *ota, Hal *hal)
{
    m_modem = modem;
    m_display = display;
    m_ledManager = ledManager;
    m_hal = hal;
    m_console = console;
    m_mqtt = mqtt;
    m_state = state;
    m_ota = ota;
    nuviotClient = this;
    m_sysConfig = sysConfig;

    mqtt->setMessageReceivedCallback(messagePublished_CallBack);
}

void NuvIoTClient::sendStatusUpdate(String currentState, String nextAction, String title, int afterDelay)
{
    m_display->drawStr(title.c_str(), currentState.c_str());
    delay(1000);
    m_display->drawStr(title.c_str(), nextAction.c_str());

    if (afterDelay > 0)
    {
        delay(afterDelay);
    }
}

void NuvIoTClient::enableGPS(bool enabled)
{
    m_gpsEnabled = enabled;
}

void NuvIoTClient::sendStatusUpdate(String currentState, String nextAction)
{
    sendStatusUpdate(currentState, nextAction, "Commo starting", 0);
}

void NuvIoTClient::handleError(String errorCode, String message)
{
    m_display->drawStr("ERROR", errorCode.c_str(), message.c_str());
    m_console->printError("err=" + errorCode + "," + message + ",abort");
    m_ledManager->setErrFlashRate(2);
    m_lastError = errorCode;

    delayAndCheckState(5000);
}

void NuvIoTClient::handleWarning(String errorCode, String message, int retryCount)
{
    m_display->drawStr("WARNING", errorCode.c_str(), message.c_str(), ("Retry Count " + String(retryCount)).c_str());
    m_console->printWarning("warning=" + errorCode + "," + message + ",retrycount=" + String(retryCount));
    m_ledManager->setErrFlashRate(8);
    m_lastWarning = errorCode;
    delayAndCheckState(1000);
}

void NuvIoTClient::delayAndCheckState(long ms)
{
    while (ms-- > 0)
    {
        m_state->loop();
        delay(1);
    }
}

bool NuvIoTClient::ConnectToAPN(bool transparentMode, bool shouldConnectToAPN, unsigned long baudRate)
{    
    sendStatusUpdate("Ready", "Connecting to Modem");
    delay(1000);

    m_modem->isModemOnline();

    m_console->println("connect=starting.");

    int retryCount = 0;
    while (!m_modem->isModemOnline() && retryCount < 10)
    {
        handleWarning("MODEM001", "Fail - Find modem.", retryCount++);
    }

    if (retryCount == 10)
    {
        handleError("MODEM001", "Fail - Find modem");
        return false;
    }

    retryCount = 0;
    m_ledManager->setErrFlashRate(0);
    m_display->drawStr("Modem Online", "Resetting Modem");
    m_console->println("modem=online;");
    m_modem->setBaudRate(baudRate);
    m_modem->enableErrorMessages();
    sendStatusUpdate("Ready", "Resetting Modem");

    while (!m_modem->resetModem() && retryCount < 10)
    {
        handleWarning("MODEM002", "Fail - reset modem", retryCount++);
    }

    if (retryCount == 10)
    {
        handleError("MODEM002", "Fail - reset modem");
        return false;
    }

    retryCount = 0;
    m_ledManager->setErrFlashRate(0);
    m_console->println("modem=reset;");
    m_display->drawStr("Modem Reset", "Getting SIMID");

    String simId = m_modem->getSIMId();
    if (simId == "")
    {
        handleError("COMM0004", "Could not find SIM.");
        while (1)
            ;
    }

    retryCount = 0;
    m_display->drawStr("COMMS", "SIMID", simId.c_str());
    m_console->println("sim=" + simId + ";");
    delayAndCheckState(1000);
    m_display->drawStr("Got SIMID", "Initialize Modem");
    m_ledManager->setErrFlashRate(0);

    while (!m_modem->init() && retryCount < 10)
    {
        handleWarning("COMMS005", "Fail - init modem.", retryCount++);
    }

    if (retryCount == 10)
    {
        handleError("COMMS005", "Fail - init modem.");
        return false;
    }

    retryCount = 0;
    m_console->println("modem=initialized;");
    m_display->drawStr("COMM5", "Initialize Modem");
    delayAndCheckState(1000);
    m_ledManager->setErrFlashRate(0);

    if (this->m_gpsEnabled)
    {

        while (!m_modem->initGPS() && retryCount < 10)
        {
            handleWarning("COMMS006", "Fail - init GPS.", retryCount++);
        }

        if (retryCount == 10)
        {
            handleError("COMMS006", "Fail - init GPS");
            return false;
        }

        retryCount = 0;
        m_console->println("gps=online;");
        m_display->drawStr("COMM6", "GPS Read");
        delayAndCheckState(1000);
        m_ledManager->setErrFlashRate(0);
    }

    if (transparentMode)
    {
        sendStatusUpdate("Modem Reset", "Enabled transpent mode");

        retryCount = 0;

        while (!m_modem->enableTransparentMode() && retryCount < 10)
        {
            handleWarning("COMMS007", "Fail - set transparent mode.", retryCount++);
        }

        if (retryCount == 10)
        {
            handleError("COMMS007", "Fail - set transparent mode.");
            return false;
        }

        retryCount = 0;
        m_console->println("transparentmode=set;");
        m_display->drawStr("COMM7", "Set transparent mode.");
        delayAndCheckState(1000);
        m_ledManager->setErrFlashRate(0);

        sendStatusUpdate("Enabled transparent mode", "Connecting to APN");
    }
    else
    {
        sendStatusUpdate("Modem Reset", "Connecting to APN");
    }

    if (shouldConnectToAPN)
    {
        retryCount = 0;

        while (!m_modem->connect("hologram", "", "") && retryCount < 10)
        {
            handleWarning("COMMS0008", "Failed to connect to APN", retryCount++);
            delayAndCheckState(1000);
        }

        if (retryCount == 10)
        {
            handleError("COMMS008", "Failed to connect to APN");
            return false;
        }

        retryCount = 0;
        m_console->println("apn=connected;");
        m_display->drawStr("COMM008", "Connected to APN");
        delayAndCheckState(1000);
        m_ledManager->setErrFlashRate(0);
    }

    return true;
}

#define RETRY_COUNT 2

bool NuvIoTClient::Connect(bool isReconnect, unsigned long baudRate)
{
    m_console->setVerboseLogging(m_state->getVerboseLogging());

    bool transparentMode = false;

    if(!m_modem->isServiceConnected() || !isReconnect)
    {
        m_console->println("serviceconnected=false; // will connect to GRPS");
        if (!ConnectToAPN(transparentMode, true, baudRate))
        {
            return false;
        }
    }
    else
    {
        m_console->println("serviceconnected=true; // continue to connect to MQTT");
    }

    int retryCount = 0;

    m_state->loop();

    retryCount = 0;
    sendStatusUpdate("Connected to APN", "Connecting to MQTT");
    while (!m_modem->connectServer(m_sysConfig->SrvrHostName, "1883") && retryCount < RETRY_COUNT)
    {
        handleWarning("MQTT001", "Failed to connect to mqtt server: " + m_sysConfig->SrvrHostName, retryCount++);
        delayAndCheckState(1000);
    }

    if (retryCount == RETRY_COUNT)
    {
        handleError("MQTT001", "Failed to connect to mqtt server: " + m_sysConfig->SrvrHostName);
        return false;
    }

    retryCount = 0;
    m_console->println("mqtt=connected;");
    sendStatusUpdate("MQTT Connected", "MQTT Authorized");
    delayAndCheckState(1000);
    m_ledManager->setErrFlashRate(0);

    while (!m_mqtt->connect(m_sysConfig->SrvrUID, m_sysConfig->SrvrPWD, m_sysConfig->DeviceId) && retryCount < RETRY_COUNT)
    {
        handleWarning("MQTT002", "Failed to authenticate to m_mqtt server: " + m_sysConfig->SrvrHostName, retryCount++);
        delayAndCheckState(1000);
    }

    if (retryCount == RETRY_COUNT)
    {
        handleError("MQTT002", "Failed to authenticate to m_mqtt: " + m_sysConfig->SrvrHostName);
        return false;
    }

    retryCount = 0;
    m_console->println("mqtt=authorized;");
    ;
    sendStatusUpdate("MQTT Authorized", "Publish Connect");
    delayAndCheckState(1000);
    m_ledManager->setErrFlashRate(0);

    String onlinePayload = "{'rssi':" + String(m_modem->getSignalQuality()) + ",'reconnect':" + String(isReconnect) + "}";

    while (!m_mqtt->publish("nuviot/srvr/dvcsrvc/" + m_sysConfig->DeviceId + "/online", onlinePayload, QOS0) && retryCount < RETRY_COUNT)
    {
        handleWarning("MQTT003", "Failed publish nuviot/dvconline.", retryCount++);
        delayAndCheckState(1000);
    }

    if (retryCount == RETRY_COUNT)
    {
        handleError("MQTT003", "Failed publish [nuviot/dvconline].");
        return false;
    }

    retryCount = 0;
    m_console->println("mqtt=publishonline;");
    sendStatusUpdate("Published Connect", "Subscribe to sys msgs");
    delayAndCheckState(1000);
    m_ledManager->setErrFlashRate(0);

    while (m_mqtt->subscribe("nuviot/dvcsrvc/" + m_sysConfig->DeviceId + "/#", QOS0) == -1 && retryCount < RETRY_COUNT)
    {
        handleWarning("MQTT004", "Failed subscribe to [nuviot/dvcsrvc].", retryCount++);
        delayAndCheckState(1000);
    }

    if (retryCount == RETRY_COUNT)
    {
        handleError("MQTT004", "Failed subscribe to [nuviot/dvcsrvc].");
        return false;
    }

    retryCount = 0;
    m_console->println("mqtt=subscribed;");
    sendStatusUpdate("Subscribed to sys msgs", "Ready");
    delayAndCheckState(1000);
    m_ledManager->setErrFlashRate(0);

    return true;
}

void NuvIoTClient::messagePublished(String topic, unsigned char *payload, size_t length)
{
    m_console->println("mqttrecv=" + topic + "; // length=" + String(length));

    if (messageReceivedCallback != NULL)
    {
        messageReceivedCallback(topic, payload, length);
    }

    String parts[10];
    int partIdx = 0;
    int start = 0;
    int topicLen = topic.length();
    for (int idx = 0; idx < topicLen; ++idx)
    {
        switch (topic[idx])
        {
        case '/':
            topic[idx] = 0x00;
            String segment = String(&topic[start]);
            parts[partIdx++] = segment;

            start = idx + 1;
            break;
        }
    }

    // Stick the final part where it belongs.
    String finalSegement = String(&topic[start]);
    parts[partIdx++] = finalSegement;

    for (int idx = 0; idx < partIdx; ++idx)
    {
        m_console->printVerbose(parts[idx]);
    }

    if (partIdx >= 4)
    {
        if (parts[0] == "nuviot" &&
            parts[1] == "dvcsrvc")
        {
            /* ok if we were on a small micro-controller, I'd probably be less tempted 
             * to take a short cut and use a string...we'll we aren't.
             */
            String action = parts[3];
            String fieldType = "";
            String field = "";
            String value = "";

            if (action == "setproperty")
            {
                if (partIdx >= 5)
                {
                    int propertyState = 0;
                    for (int i = 0; i < length; i++)
                    {
                        if (((char)payload[i]) == ',')
                        {
                            propertyState = 1;
                        }
                        else if (((char)payload[i]) == '=')
                        {
                            propertyState = 2;
                        }
                        else
                        {
                            if (propertyState == 0)
                            {
                                fieldType += (char)payload[i];
                            }
                            else if (propertyState == 1)
                            {
                                field += (char)payload[i];
                            }
                            else if (propertyState == 2)
                            {
                                value += (char)payload[i];
                            }
                        }
                    }

                    m_state->updateProperty(fieldType, field, value);
                }
            }
            else if (action == "properties")
            {
                if (partIdx >= 5)
                {
                    if (parts[3] == "properties" &&
                        parts[4] == "query")
                    {
                        String payload = m_state->getRemoteProperties();
                        String topic = "nuviot/srvr/dvcsrvc/" + m_sysConfig->DeviceId + "/state";
                        m_mqtt->publish(topic, payload, QOS0);
                    }
                }
            }
            else if (action == "restart")
            {
                m_hal->restart();
            }
            else if (action == "update")
            {
                if (partIdx >= 4)
                {
                    m_modem->exitDataMode();
                    String url = "http://api.nuviot.com/api/firmware/download/" + parts[4];
                    m_console->println("Request Download.");
                    m_console->println(url);
                    m_ota->start(url);
                }
            }
        }
    }
}