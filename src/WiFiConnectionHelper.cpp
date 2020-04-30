#include "WiFiConnectionHelper.h"

WiFiConnectionHelper::WiFiConnectionHelper(WiFiClient *client, Display *display, NuvIoTState *state)
{
    m_display = display;
    m_State = state;
    m_client = client;
}

void WiFiConnectionHelper::loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connect(true);
    }
}

void WiFiConnectionHelper::connect(bool isReconnect)
{
    int status = WiFi.status();

    int attempt = 0;
    int idx = 0;

    WiFi.begin(m_State->getWiFiSSID().c_str(), m_State->getWiFiPassword().c_str());
    Serial.println(m_State->getWiFiSSID() + " " + m_State->getWiFiPassword());

    while (status != WL_CONNECTED)
    {
        attempt++;

        if (attempt % 20 == 0)
        {
            WiFi.disconnect();
            Serial.println(m_State->getWiFiSSID() + " " + m_State->getWiFiPassword());
            WiFi.begin(m_State->getWiFiSSID().c_str(), m_State->getWiFiPassword().c_str());
        }

        delay(500);

        m_State->loop();

        m_display->clearBuffer();
        m_display->drawString(0, 0, "NuvIoT");
        m_display->drawString(0, 16, isReconnect ? "Reconnecting WiFi" : "Connecting WiFi");
        m_display->drawString(0, 32, m_State->getWiFiSSID().c_str());

        delay(2500);

        if (idx == 0)
            m_display->drawString(90, 0, "/");
        else if (idx == 1)
            m_display->drawString(90, 0, "-");
        else if (idx == 2)
            m_display->drawString(90, 0, "\\");

        m_display->drawString(70, 0, String(attempt).c_str());

        idx++;
        if (idx == 3)
            idx = 0;

        status = WiFi.status();
        switch (status)
        {
        case WL_IDLE_STATUS:
            m_display->drawString(0, 48, "Idle");
            break;
        case WL_NO_SHIELD:
            m_display->drawString(0, 48, "No Shield");
            break;
        case WL_NO_SSID_AVAIL:
            m_display->drawString(0, 48, "No SSID Available");
            break;
        case WL_SCAN_COMPLETED:
            m_display->drawString(0, 48, "Scan Completed");
            break;
        case WL_CONNECTED:
            m_display->drawString(0, 48, "Connected");
            break;
        case WL_CONNECT_FAILED:
            m_display->drawString(0, 48, "Connection Failed");
            break;
        case WL_CONNECTION_LOST:
            m_display->drawString(0, 48, "Connection Lost");
            break;
        case WL_DISCONNECTED:
            m_display->drawString(0, 48, "Disconnected");
            break;
        }

        m_display->sendBuffer();
    }

    m_display->clearBuffer();
    m_display->drawString(0, 0, "Connected to:");
    m_display->drawString(80, 0, m_State->getWiFiSSID().c_str());
    m_display->sendBuffer();
    Serial.println("Connected to WiFi");
}

bool WiFiConnectionHelper::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}


String WiFiConnectionHelper::getIPAddress() {
    return WiFi.localIP().toString();
    
}

String WiFiConnectionHelper::getMACAddress() {
    return WiFi.macAddress();
}

void WiFiConnectionHelper::disconnect()
{
    WiFi.disconnect();
}

void WiFiConnectionHelper::setup()
{
    connect(false);
}