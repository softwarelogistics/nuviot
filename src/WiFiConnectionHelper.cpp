#include "WiFiConnectionHelper.h"

WiFiConnectionHelper::WiFiConnectionHelper(WiFiClient *client, Display *display, LedManager *ledManager, 
                                           NuvIoTState *state, Hal *hal, Console *console, SysConfig *sysConfig)
{
    m_display = display;
    m_sysConfig = sysConfig;
    m_client = client;
    m_state = state;
    m_console = console;
    m_hal = hal;
    m_ledManager = ledManager;
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

void WiFiConnectionHelper::loop()
{
    // If WiFi isn't enabled, to bother trying to connect.
    if(!m_sysConfig->WiFiEnabled)
    {
        return;
    }

    // If we aren't commissioned, then don't bother trying to connect.
    if(!m_sysConfig->Commissioned)
    {
        return;
    }

    // While in configuration mode, we shut off wifi,
    // it makes bluetooth work a little better since the ESP
    // shares the same radio for WiFi and BT.
    if (m_state->getIsConfigurationModeActive())
    {
        WiFi.disconnect();
        return;
    }

    int status = WiFi.status();

    // If we are connected, life is good, just mark us as connected and bail.
    if(status == WL_CONNECTED)
    {
        if(m_wifiState != NuvIoTWiFi_Connected)
        {
            m_display->clearBuffer();
            m_display->drawString(0, 0, "Connected to:");
            m_display->drawString(80, 0, m_sysConfig->WiFiSSID.c_str());
            m_display->sendBuffer();
            m_console->println("wifi=connected;");
            

            IPAddress ip = WiFi.localIP();

            String sIPAddress = "";
            for (int i=0; i<4; i++)
                sIPAddress += i  ? "." + String(ip[i]) : String(ip[i]);

            m_console->println("wifi_ipaddress=" + sIPAddress + ";");
            m_ledManager->setOnlineFlashRate(10);

            m_wifiState = NuvIoTWiFi_Connected;
        }
        
        // This is the state we normally want to be in.
        return;
    }

    if(m_wifiState == NuvIoTWiFi_NotConnected)
    {
        m_console->printError("wifi=notconnected; // Starting to connect.");
        connect(false);
    }

    // If we are connected, then that means we have lost our connection.
    if(m_wifiState == NuvIoTWiFi_Connected)
    {
        m_console->printError("wifi=lostconnection; // Starting to reconnecting.");        
        connect(true);
    }

    m_attempt++;
    m_display->clearBuffer();
    m_display->drawString(0, 0, "NuvIoT");
    m_display->drawString(0, 16, "Connecting WiFi");
    m_display->drawString(0, 32, m_sysConfig->WiFiSSID.c_str());

    if (m_spinnerIndex == 0)
        m_display->drawString(90, 0, "/");
    else if (m_spinnerIndex == 1)
        m_display->drawString(90, 0, "-");
    else if (m_spinnerIndex == 2)
        m_display->drawString(90, 0, "\\");

    m_display->drawString(70, 0, String(m_attempt).c_str());

    m_spinnerIndex++;
    if (m_spinnerIndex == 3)
        m_spinnerIndex = 0;

    String statusMsg = getWiFiStatus(status);
    m_display->drawString(0, 48, statusMsg);
    m_display->sendBuffer();
    m_console->println("wifi=connecting; // attempt=" + String(m_attempt) + ", status=" + statusMsg);

    if(m_attempt == 60)
    {
        m_hal->restart();
    }    
}


void WiFiConnectionHelper::connect(bool isReconnect)
{
    m_isReconnect = isReconnect;
    m_wifiState = NuvIoTWiFi_Connecting;
    m_console->println("wifi=connect;");

    m_attempt = 0;

    int status = WiFi.status();

    m_ledManager->setOnlineFlashRate(2);

   // WiFi.begin(m_sysConfig->WiFiSSID.c_str(), m_sysConfig->WiFiPWD.c_str());
    if(status == WL_CONNECTED)
    {
        WiFi.disconnect();
    }

    String statusMsg = "connecting";

    if (!WiFi.enableSTA(true))
    {
        statusMsg = "Fail enable STA";
        m_console->println("wifi=connecting; // attempt=" + String(m_attempt) + ", status=fail enable STA");
        m_display->drawString(0, 48, "Fail enable STA");
        m_display->sendBuffer();
        m_ledManager->setErrFlashRate(2);
        return;
    }

    m_console->println("wifi=connecting; // ssid=" + m_sysConfig->WiFiSSID + ", pwd=" + m_sysConfig->WiFiPWD.c_str());   
    WiFi.begin(m_sysConfig->WiFiSSID.c_str(), m_sysConfig->WiFiPWD.c_str());

    m_ledManager->setErrFlashRate(6);
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