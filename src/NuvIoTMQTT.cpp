#include "NuvIoTMQTT.h"

#include <WiFi.h>
#include <Arduino.h>

static NuvIoTMQTT *mqttInstance;

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    mqttInstance->handleMqttCallback(topic, payload, length);
}

NuvIoTMQTT::NuvIoTMQTT(WiFiConnectionHelper *wifiConnection, WiFiClient *client, Display *display, OtaServices *ota, Hal *hal, NuvIoTState *state)
{
    m_client = client;
    m_mqtt = new PubSubClient(*client);
    m_wifi = wifiConnection;
    m_display = display;
    m_state = state;
    m_hal = hal;
    m_ota = ota;
    m_mqtt->disconnect();

    mqttInstance = this;
}

void NuvIoTMQTT::connect()
{
    int idx = 0;
    int attempt = 1;

    m_mqtt->disconnect();

    while (!m_mqtt->connected())
    {
        ++idx;

        m_display->clearBuffer();
        m_display->drawString(0, 0, "Connecting MQTT");
        m_display->drawString(0, 16, m_state->getHostName().c_str());
        m_display->drawString(0, 32, "Attempt");
        m_display->drawString(90, 32, String(attempt).c_str());
        m_display->drawString(0, 48, resolveConnectFail().c_str());

        if (idx == 0 || idx == 4)
            m_display->drawString(60, 32, "|");
        else if (idx == 1 || idx == 5)
            m_display->drawString(60, 32, "/");
        else if (idx == 2 || idx == 6)
            m_display->drawString(60, 32, "-");
        else if (idx == 3)
            m_display->drawString(60, 32, "\\");
        else if (idx == 7)
        {
            m_display->drawString(60, 32, "\\");
            idx = 0;
        }

        m_display->sendBuffer();

        m_wifi->loop();

        m_state->loop();
        m_mqtt->loop();

        IPAddress remote_addr;
        (WiFi.hostByName(m_state->getHostName().c_str(), remote_addr));
        Serial.println(m_state->getHostName() + "=" + remote_addr.toString() + " with device id: " + m_state->getDeviceId());

        //m_mqtt->setServer(m_state->getHostName().c_str(), 1883);
        m_mqtt->setServer(remote_addr, 1883);
        bool connectResult;
        if (m_state->getIsAnonymous())
        {
            connectResult = m_mqtt->connect(m_state->getDeviceId().c_str());
        }
        else
        {
            Serial.println(m_state->getHostName() + " " + m_state->getDeviceId() + " " + remote_addr.toString() + " " + m_state->getHostUserName() + " " + m_state->getHostPassword());
            connectResult = m_mqtt->connect(m_state->getDeviceId().c_str(), m_state->getHostUserName().c_str(), m_state->getHostPassword().c_str());
        }

        m_mqtt->loop();

        if (m_client->connected())
        {
            Serial.println("looks like client is connected.");
        }
        else
        {
            Serial.println("looks like we are not connected.");
        }

        if (connectResult)
        {
            for (int idx = 0; idx < m_subscrptionCount; ++idx)
            {
                m_mqtt->subscribe(m_subscriptions[idx].c_str());
                Serial.println("Added subscription: " + m_subscriptions[idx]);
            }

            m_mqtt->setCallback(mqttCallback);

            m_display->clearBuffer();
            m_display->drawString(0, 0, "Success");
            m_display->drawString(0, 16, m_state->getHostName().c_str());
            m_display->drawString(0, 32, "Connected MQTT");
            m_display->sendBuffer();
            delay(1500);
        }
        else
        {
            delay(2500);
        }

        ++attempt;
    }
}

void NuvIoTMQTT::handleMqttCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    String parts[10];
    int partIdx = 0;
    int start = 0;
    int topicLen = strlen(topic);
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
                    int state = 0;
                    for (int i = 0; i < length; i++)
                    {
                        if(((char)payload[i]) == ','){
                            state = 1;
                        }
                        else if(((char)payload[i]) == '=') {
                            state = 2;
                        }else {
                            if(state == 0) {
                                fieldType += (char)payload[i];
                            }
                            else if(state == 1){
                                field += (char)payload[i];
                            }
                            else if(state == 2) {
                                value += (char)payload[i];
                            }
                        }
                    }

                    m_state->updateProperty(fieldType, field, value);
                }
            }
            else if (action == "properties")
            {
                if(partIdx >= 5){
                    if(parts[3] == "properties" &&
                       parts[4] == "query") {
                           String payload = m_state->queryState();
                           Serial.println(payload);
                           String topic = "nuviot/srvr/dvcsrvc/" + m_state->getDeviceId() + "/state";
                           m_mqtt->publish_P(topic.c_str(), (uint8_t *)payload.c_str(), payload.length(), false);
                       }
                }
            }
            else if (action == "restart")
            {
                m_hal->restart();
            }
            else if(action == "update") {
                if(partIdx >= 4) {
                    String url = "http://api.nuviot.com/api/firmware/download/" + parts[4];
                    m_ota->start(url);
                }
            }
        }
    }

    Serial.println();
}

void NuvIoTMQTT::addSubscriptions(String subscription)
{
    m_subscriptions[m_subscrptionCount++] = subscription;
}

void NuvIoTMQTT::setup()
{
}

String NuvIoTMQTT::resolveConnectFail()
{
    int state = m_mqtt->state();
    String connectionStateMessage;
    switch (state)
    {
    case MQTT_CONNECTION_TIMEOUT:
        connectionStateMessage = "Connection Timeout";
        break;
    case MQTT_CONNECTION_LOST:
        connectionStateMessage = "Connection Lost";
        break;
    case MQTT_CONNECT_FAILED:
        connectionStateMessage = "Connect Failed";
        break;
    case MQTT_DISCONNECTED:
        connectionStateMessage = "Client Disconnected.";
        break;
    case MQTT_CONNECTED:
        connectionStateMessage = "Connected.";
        break;
    case MQTT_CONNECT_BAD_PROTOCOL:
        connectionStateMessage = "Bad Protocol.";
        break;
    case MQTT_CONNECT_BAD_CLIENT_ID:
        connectionStateMessage = "Bad Client Id.";
        break;
    case MQTT_CONNECT_UNAVAILABLE:
        connectionStateMessage = "Connect Unavailable.";
        break;
    case MQTT_CONNECT_BAD_CREDENTIALS:
        connectionStateMessage = "Bad Credentials.";
        break;
    case MQTT_CONNECT_UNAUTHORIZED:
        connectionStateMessage = "Connect Unauthorized.";
        break;
    }

    return connectionStateMessage;
}

void NuvIoTMQTT::loop()
{
    if (!m_mqtt->connected())
    {
        Serial.println("Attempting MQTT connection...");
        m_display->clearBuffer();
        m_display->drawString(0, 0, "MQTT Not Connected");
        m_display->drawString(0, 16, resolveConnectFail().c_str());
        m_display->sendBuffer();
        delay(1500);
        connect();
    }
    else
    {
        m_mqtt->loop();
    }
}

void NuvIoTMQTT::publish(String topic, String payload)
{
    m_mqtt->publish_P(topic.c_str(), (uint8_t *)payload.c_str(), payload.length(), false);
}