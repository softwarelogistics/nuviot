#include "WiFiConnectionHelper.h"

WiFiConnectionHelper::WiFiConnectionHelper(WiFiClient *client, Display *display, NuvIoTState *state, Console *console, SysConfig *sysConfig)
{
    m_display = display;
    m_sysConfig = sysConfig;
    m_client = client;
    m_state = state;
    m_console = console;
}

void WiFiConnectionHelper::loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        m_console->printError("wifi=lostconnection; // Starting to reconnecting.");
        connect(true);
    }
}

String WiFiConnectionHelper::getWiFiStatus(int status)
{
    String statusMsg = "";

    switch (status)
    {
    case WL_IDLE_STATUS:
        statusMsg = "Idle";
        break;
    case WL_NO_SHIELD:
        statusMsg = "No Shield";
        break;
    case WL_NO_SSID_AVAIL:
        statusMsg = "No SSID Available";
        break;
    case WL_SCAN_COMPLETED:
        statusMsg = "Scan Completed";
        break;
    case WL_CONNECTED:
        statusMsg = "Connected";
        break;
    case WL_CONNECT_FAILED:
        statusMsg = "Connection Failed";
        break;
    case WL_CONNECTION_LOST:
        statusMsg = "Connection Lost";
        break;
    case WL_DISCONNECTED:
        statusMsg = "Disconnected";
        break;
    }

    return statusMsg;
}

void WiFiConnectionHelper::connect(bool isReconnect)
{
    m_console->println("Start WiFi Connection.");

    int status = WiFi.status();

    int attempt = 0;
    int idx = 0;

   // WiFi.begin(m_sysConfig->WiFiSSID.c_str(), m_sysConfig->WiFiPWD.c_str());
    m_console->println("wifi=connecting; // ssid=" + m_sysConfig->WiFiSSID + ", pwd=" + m_sysConfig->WiFiPWD.c_str());

    while (status != WL_CONNECTED)
    {
        m_state->loop();

        if (m_state->getIsConfigurationModeActive())
        {
            WiFi.disconnect();
            delay(5);
        }
        else
        {
            attempt++;
            WiFi.disconnect();
            m_display->clearBuffer();
            m_display->drawString(0, 0, "NuvIoT");
            m_display->drawString(0, 16, isReconnect ? "Reconnecting WiFi" : "Connecting WiFi");
            m_display->drawString(0, 32, m_sysConfig->WiFiSSID.c_str());

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


            String statusMsg = "connecting";

            if (!WiFi.enableSTA(true))
            {
                statusMsg = "Fail enable STA";
                m_console->println("wifi=connecting; // attempt=" + String(attempt) + ", status=fail enable STA");
                m_display->drawString(0, 48, "Fail enable STA");
                m_display->sendBuffer();
                delay(500);
            }
            else
            {
                int status = WiFi.begin(m_sysConfig->WiFiSSID.c_str(), m_sysConfig->WiFiPWD.c_str());

                delay(500);
                statusMsg = getWiFiStatus(status);
                m_display->drawString(0, 48, statusMsg);
                m_display->sendBuffer();
                m_console->println("wifi=connecting; // attempt=" + String(attempt) + ", status=" + statusMsg);
            }
        }
    }

    m_display->clearBuffer();
    m_display->drawString(0, 0, "Connected to:");
    m_display->drawString(80, 0, m_sysConfig->WiFiSSID.c_str());
    m_display->sendBuffer();
    m_console->println("wifi=connected;");
}

int WiFiConnectionHelper::getRSSI()
{
    return WiFi.RSSI();
}

bool WiFiConnectionHelper::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

String WiFiConnectionHelper::getIPAddress()
{
    return WiFi.localIP().toString();
}

String WiFiConnectionHelper::getMACAddress()
{
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