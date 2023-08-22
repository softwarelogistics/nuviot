#include "NuvIoTMQTT.h"

#include <WiFi.h>
#include <Arduino.h>

static NuvIoTMQTT *mqttInstance;

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    mqttInstance->handleMqttCallback(topic, payload, length);
}

NuvIoTMQTT::NuvIoTMQTT(WiFiConnectionHelper *wifiConnection, Console *console, WiFiClient *client, Display *display, OtaServices *ota, Hal *hal, NuvIoTState *state, SysConfig *sysConfig)
{
    m_console = console;
    m_sysConfig = sysConfig;
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
    if (m_sysConfig->SrvrHostName == "")
    {
        m_console->printWarning("wifimqtt=notconfigured; // No mqtt host configured, will not attempt to connect.");
        return;
    }

    m_mqtt->disconnect();
    m_wifi->loop();

    m_display->clearBuffer();
    m_display->drawString(0, 0, "Connecting MQTT");
    m_display->drawString(0, 16, m_sysConfig->SrvrHostName.c_str());
    m_display->drawString(0, 32, "Attempt");
    m_display->drawString(90, 32, String(m_connectAttempt).c_str());
    m_display->drawString(0, 48, resolveConnectFail().c_str());

    if (m_spinnerIndex == 0 || m_spinnerIndex == 4)
        m_display->drawString(60, 32, "|");
    else if (m_spinnerIndex == 1 || m_spinnerIndex == 5)
        m_display->drawString(60, 32, "/");
    else if (m_spinnerIndex == 2 || m_spinnerIndex == 6)
        m_display->drawString(60, 32, "-");
    else if (m_spinnerIndex == 3)
        m_display->drawString(60, 32, "\\");
    else if (m_spinnerIndex == 7)
    {
        m_display->drawString(60, 32, "\\");
        m_spinnerIndex = 0;
    }

    m_display->sendBuffer();

    m_state->loop();
    m_mqtt->loop();

    IPAddress remote_addr;
    (WiFi.hostByName(m_sysConfig->SrvrHostName.c_str(), remote_addr));
    m_console->println("wifimqtt=connecting; // host=" + m_sysConfig->SrvrHostName + "; addr=" + remote_addr.toString() + "; deviceid=" + m_sysConfig->DeviceId + "; uid=" + m_sysConfig->SrvrUID + "; pwd=" + m_sysConfig->SrvrPWD);

    m_mqtt->setServer(remote_addr, 1883);
    bool connectResult = m_state->getIsAnonymous() ? m_mqtt->connect(m_sysConfig->DeviceId.c_str()) : m_mqtt->connect(m_sysConfig->DeviceId.c_str(), m_sysConfig->SrvrUID.c_str(), m_sysConfig->SrvrPWD.c_str());

    if (connectResult)
    {
        m_mqtt->loop();

        publish("nuviot/srvr/dvcsrvc/" + m_sysConfig->DeviceId + "/online", "firmwareVersion=" + m_state->getFirmwareVersion() + ",firmwareSku=" + m_state->getFirmwareSKU() + ",ipAddress=" + m_wifi->getIPAddress());

        m_console->println("wifimqtt=mqttconnected; // host=" + m_sysConfig->SrvrHostName + ".");
        for (int idx = 0; idx < m_subscriptionCount; ++idx)
        {
            if (m_mqtt->subscribe(m_subscriptions[idx].c_str()))
                m_console->println("wifimqtt-subscribed=success; // subscription=" + m_subscriptions[idx]);
            else
                m_console->printError("wifimqtt=subscribed=failed; // subscription=" + m_subscriptions[idx]);
        }

        String dvcServiceSubscription = "nuviot/dvcsrvc/" + m_sysConfig->DeviceId + "/#";
        if (m_mqtt->subscribe(dvcServiceSubscription.c_str()))
            m_console->println("wifimqtt-subscribed=success; // subscription=" + dvcServiceSubscription);
        else
            m_console->printError("wifimqtt=subscribed=failed; // subscription=" + dvcServiceSubscription);

        m_mqtt->setCallback(mqttCallback);

        m_display->clearBuffer();
        m_display->drawString(0, 0, "Success");
        m_display->drawString(0, 16, m_sysConfig->SrvrHostName.c_str());
        m_display->drawString(0, 32, "Connected MQTT");
        m_display->sendBuffer();
        delay(1500);
        m_lastConnectAttempt = 0;
        m_connectAttempt = 0;
        m_spinnerIndex = 0;
    }
    else
    {
        m_console->printError("wifimqtt=failedmqttconnected; // host=" + m_sysConfig->SrvrHostName + ".");
        m_lastConnectAttempt = millis();
        ++m_connectAttempt;
    }
}

void NuvIoTMQTT::disconnect()
{
    m_mqtt->disconnect();
    m_wifi->disconnect();
}

void NuvIoTMQTT::setMessageReceivedCallback(void (*callback)(String topic, byte *buffer, size_t len))
{
    m_callback = callback;
}

void NuvIoTMQTT::handleMqttCallback(char *topic, byte *payload, unsigned int length)
{
    String strTopic = String(topic);

    m_console->println("wifimqtt=receive; // topic=" + strTopic + ".");

    String strPayload = "";

    for (int i = 0; i < length; i++)
    {
        strPayload += (char)payload[i];
    }

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
            parts[partIdx++] = segment;

            start = idx + 1;
            break;
        }
    }

    parts[partIdx++] = String(&topic[start]);

    if (length > 0)
    {
        m_console->printVerbose("Payload:");
        m_console->printVerbose(strPayload);
        m_console->printVerbose("");
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
                        if (((char)payload[i]) == ',')
                        {
                            state = 1;
                        }
                        else if (((char)payload[i]) == '=')
                        {
                            state = 2;
                        }
                        else
                        {
                            if (state == 0)
                            {
                                fieldType += (char)payload[i];
                            }
                            else if (state == 1)
                            {
                                field += (char)payload[i];
                            }
                            else if (state == 2)
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
                        String payload = m_state->queryFirmwareVersion();
                        String topic = "nuviot/srvr/dvcsrvc/" + m_sysConfig->DeviceId + "/state";
                        m_mqtt->publish_P(topic.c_str(), (uint8_t *)payload.c_str(), payload.length(), false);
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
                    String url = "http://firmware.nuviot.com:14236/api/firmware/download/" + parts[4];
                    publish(String("nuviot/srvr/dvcsrvc/" + m_sysConfig->DeviceId + "/fwupdate/start"), String("{'url':'" + url + "'}"));

                    if(m_wifi->isConnected()) {
                        m_ota->downloadOverWiFi(url);
                    }
                    else {                        
                        m_ota->start(url);                        
                    }
                }
            }
        }
    }

    if (m_callback != NULL)
    {
        m_callback(strTopic, payload, length);
    }
}

void NuvIoTMQTT::addSubscriptions(String subscription)
{
    m_console->println("wifimqtt=addsubscription; // topic=" + subscription + ".");
    m_subscriptions[m_subscriptionCount++] = subscription;
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
        return "Connection Timeout";
    case MQTT_CONNECTION_LOST:
        return "Connection Lost";
    case MQTT_CONNECT_FAILED:
        return "Connect Failed";
    case MQTT_DISCONNECTED:
        return "Client Disconnected.";
    case MQTT_CONNECTED:
        return "Connected.";
    case MQTT_CONNECT_BAD_PROTOCOL:
        return "Bad Protocol.";
    case MQTT_CONNECT_BAD_CLIENT_ID:
        return "Bad Client Id.";
    case MQTT_CONNECT_UNAVAILABLE:
        return "Connect Unavailable.";
    case MQTT_CONNECT_BAD_CREDENTIALS:
        return "Bad Credentials.";
    case MQTT_CONNECT_UNAUTHORIZED:
        return "Connect Unauthorized.";
    }

    return "?";
}

bool NuvIoTMQTT::isConnected()
{
    return m_mqtt->connected();
}

void NuvIoTMQTT::loop()
{
    m_wifi->loop();

    if (!m_wifi->isConnected())
    {
        m_state->setIsCloudConnected(false);
        m_console->printError("wifimqtt=notconnected; // wifi not connected will not attempt to connect to mqtt;");

        m_display->clearBuffer();
        m_display->drawString(0, 0, "Client Not Connected");
        m_display->drawString(0, 16, resolveConnectFail().c_str());
        m_display->sendBuffer();
    }
    else if (!m_mqtt->connected())
    {
        m_state->setWiFiRSSI(m_wifi->getRSSI());
        m_state->setIsCloudConnected(false);
        m_console->printError("wifimqtt=notconnected;");

        m_display->clearBuffer();
        m_display->drawString(0, 0, "MQTT Not Connected");
        m_display->drawString(0, 16, resolveConnectFail().c_str());
        m_display->sendBuffer();
        delay(250);

        connect();
    }
    else
    {
        m_state->setIsCloudConnected(true);
        /* this client automatically does ping */
        m_mqtt->loop();
    }
}

void NuvIoTMQTT::publish(String topic, byte *buffer, uint16_t length)
{
    if (isConnected())
        m_mqtt->publish_P(topic.c_str(), buffer, length, false);
}

void NuvIoTMQTT::publish(String topic, String payload)
{
    if (isConnected())
        m_mqtt->publish_P(topic.c_str(), (uint8_t *)payload.c_str(), payload.length(), false);
}

void NuvIoTMQTT::sendIOValues(IOValues *values)
{
    String topic = "nuviot/srvr/dvcsrvc/" + m_sysConfig->DeviceId + "/iovalues";
    publish(topic, values->toString());
}

void NuvIoTMQTT::sendRelayStatus(RelayManager *mgr)
{
    String topic = "nuviot/srvr/dvcsrvc/" + m_sysConfig->DeviceId + "/relays";
    m_console->println(mgr->toString());
    publish(topic, mgr->toString());
}