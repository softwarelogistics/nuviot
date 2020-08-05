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

NuvIoTClient::NuvIoTClient(SIMModem *modem, MQTT *mqtt, Console *console, Display *display, NuvIoTState *state, OtaServices *ota, Hal *hal)
{
    m_modem = modem;
    m_display = display;
    m_hal = hal;
    m_console = console;
    m_mqtt = mqtt;
    m_state = state;
    m_ota = ota;
    nuviotClient = this;

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

void NuvIoTClient::sendStatusUpdate(String currentState, String nextAction)
{
    sendStatusUpdate(currentState, nextAction, "Commo starting", 0);
}

bool NuvIoTClient::ConnectToAPN(bool transparentMode, bool shouldConnectToAPN, unsigned long baudRate)
{
    m_console->setVerboseLogging(m_state->getVerboseLogging());
    sendStatusUpdate("Ready", "Connecting to Modem");
    delay(1000);

    m_modem->isModemOnline();
    
    int retryCount = 0;
    while (!m_modem->isModemOnline())
    {
        m_display->drawStr("ERROR", "COMMS001", "No m_modem->", ("Retry Count " + String(++retryCount)).c_str());
        delay(2000);
    }

    m_modem->setBaudRate(baudRate);
    
    m_modem->enableErrorMessages();

    retryCount = 0;
    m_display->drawStr("Modem Online", "Resetting Modem");

    sendStatusUpdate("Ready", "Resetting Modem");

    while (!m_modem->resetModem() && retryCount < 10)
    {
        m_display->drawStr("ERROR", "COMMS003", "Resetting m_modem->", ("Retry Count " + String(++retryCount)).c_str());
        delay(2000);
    }

    if (retryCount == 10)
        return false;

    retryCount = 0;
    m_display->drawStr("Modem Reset", "Getting SIMID");

    String simId = m_modem->getSIMId();
    if (simId != "")
    {
        m_display->drawStr("COMMS", "SIMID", simId.c_str());
        delay(1000);
    }
    else
    {
        m_display->drawStr("ERROR", "COMMS004", "Could not find SIMID", "Please check");
        while (1)
            ;
    }

    retryCount = 0;
    m_display->drawStr("Got SIMID", "Initialize Modem");

    while (!m_modem->init() && retryCount < 10)
    {
        m_display->drawStr("ERROR", "COMMS005", "Initialize Modem Settings.", ("Retry Count " + String(++retryCount)).c_str());
        delay(2000);
        return false;
    }

    if (transparentMode)
    {
        sendStatusUpdate("Modem Reset", "Enabled transpent mode");

        retryCount = 0;

        while (!m_modem->enableTransparentMode() && retryCount < 10)
        {
            m_display->drawStr("ERROR", "COMMS006", "Failed Transparent Mode.", ("Retry Count " + String(++retryCount)).c_str());
        }

        if (retryCount == 10)
            return false;

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
            m_display->drawStr("ERROR", "COMMS007", "Failed APN Connect.", ("Retry Count " + String(++retryCount)).c_str());
        }

        if (retryCount == 10)
            return false;
    }

    return true;
}

bool NuvIoTClient::Connect(bool isReconnect, unsigned long baudRate)
{
    if (!ConnectToAPN(true, true, baudRate))
    {
        return false;
    }

    int retryCount = 0;

    retryCount = 0;
    sendStatusUpdate("Connected to APN", "Connecting to MQTT");
    while (!m_modem->connectServer(m_state->getHostName(), "1883") && retryCount < 10)
    {
        m_display->drawStr("ERROR", "MQTT001", "Failed MQTT Server.", ("Retry Count " + String(++retryCount)).c_str());
    }

    if (retryCount == 10)
        return false;

    retryCount = 0;

    sendStatusUpdate("Connected to MQTT", "Auth MQTT");

    while (!m_mqtt->connect(m_state->getHostUserName(), m_state->getHostPassword(), m_state->getDeviceId()) && retryCount < 10)
    {
        m_display->drawStr("ERROR", "MQTT002", "Failed Auth m_mqtt->", ("Retry Count " + String(++retryCount)).c_str());
    }

    if (retryCount == 10)
        return false;

    sendStatusUpdate("MQTT Authorized", "Publish Connect");

    String onlinePayload = "{'rssi':" + String(m_modem->getSignalQuality()) + ",'reconnect':" + String(isReconnect) + "}";

    while (!m_mqtt->publish("hvac/online/" + m_state->getDeviceId(), onlinePayload, QOS0) && retryCount < 10)
    {
        m_display->drawStr("ERROR", "MQTT003", "Failed publish Connect.", ("Retry Count " + String(++retryCount)).c_str());
    }

    if (retryCount == 10)
        return false;

    sendStatusUpdate("Published Connect", "Subscribe to sys msgs");

    while (m_mqtt->subscribe("nuviot/dvcsrvc/" + m_state->getDeviceId() + "/#", QOS0) == -1 && retryCount < 10)
    {
        m_display->drawStr("ERROR", "MQTT004", "Failed Subscribe.", ("Retry Count " + String(++retryCount)).c_str());
    }

    if (retryCount == 10)
        return false;

    sendStatusUpdate("Subscribed to sys msgs", "Ready");

    return true;
}

void NuvIoTClient::messagePublished(String topic, unsigned char *payload, size_t length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.println("] ");

    if (topic.endsWith("testing"))
    {
        m_modem->exitTransparentMode();
        m_ota->start("http://api.nuviot.com/api/firmware/download/6AEFEA1606BB4C6CB2C5135CB42B4C77");
        return;
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
            Serial.println(segment);
            parts[partIdx++] = segment;

            start = idx + 1;
            break;
        }
    }

    String segment = String(&topic[start]);
    Serial.println(segment);
    parts[partIdx++] = segment;

    for (int idx = 0; idx < partIdx; ++idx)
    {
        Serial1.println(parts[idx]);
    }

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }

    Serial.println();

    if (partIdx >= 4)
    {
        if (parts[0] == "nuviot" &&
            parts[1] == "dvcsrvc")
        {
            /* ok if we were on a micro-controller, I'd probably be less tempted 
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
                        String payload = m_state->queryState();
                        Serial.println(payload);
                        String topic = "nuviot/srvr/dvcsrvc/" + m_state->getDeviceId() + "/state";
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
                    m_modem->exitTransparentMode();
                    String url = "http://api.nuviot.com/api/firmware/download/" + parts[4];
                    m_console->println("Request Download.");
                    m_console->println(url);
                    m_ota->start(url);
                }
            }
        }
    }

    Serial.println();
}