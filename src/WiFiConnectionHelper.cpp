#include "WiFiConnectionHelper.h"
#include <HTTPClient.h>

#ifdef LCD_DISPLAY
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
#endif


WiFiConnectionHelper::WiFiConnectionHelper(WiFiClient *client, LedManager *ledManager,
                                           NuvIoTState *state, Hal *hal, Console *console, SysConfig *sysConfig)
{
    m_sysConfig = sysConfig;
    m_client = client;
    m_state = state;
    m_console = console;
    m_hal = hal;
    m_ledManager = ledManager;
}


String WiFiConnectionHelper::getWiFiStatus()
{
    switch (WiFi.status())
    {
    case WL_IDLE_STATUS:
        return "Idle - Retry: " + String(m_attempt);
    case WL_NO_SHIELD:
        return "No Shield - Retry: " + String(m_attempt);
    case WL_NO_SSID_AVAIL:
        return "No SSID Available - Retry: " + String(m_attempt);
    case WL_SCAN_COMPLETED:
        return "Scan Completed - Retry: " + String(m_attempt);
    case WL_CONNECTED:
        return "Connected";
    case WL_CONNECT_FAILED:
        return "Connection Failed - Retry: " + String(m_attempt);
    case WL_CONNECTION_LOST:
        return "Connection Lost - Retry: " + String(m_attempt);
    case WL_DISCONNECTED:
        return "Disconnected - Retry: " + String(m_attempt);
    }

    return "? - " + String(WiFi.status()) + " Retry: " + String(m_attempt);
}

String WiFiConnectionHelper::siteSurvey() {
    if(m_wifiState == NuvIoTWiFi_Connecting){
        m_console->println("wifi=scan; // currently connecting, attempting to disconnect.");
        WiFi.disconnect();
        m_console->println("wifi=scan; // currently connecting, disconnected.");
    }

    uint64_t start = millis();
    int16_t availableNetworks = WiFi.scanNetworks();
    m_console->println("wifi=scan; // time=" + String((uint16_t)(millis() - start)) + "ms, networks=" + String(availableNetworks));

    if(availableNetworks == WIFI_SCAN_RUNNING){
        m_console->println("wifi=scan failed; // already running.");
        return "Already Running=-1;";
    }

    if(availableNetworks == WIFI_SCAN_FAILED){
        m_console->println("wifi=scan failed; // scan failed.");
        return "Scan Failed=-1;";
    }

    String networks = "";
    for(int idx = 0; idx < availableNetworks; idx++){
        networks += WiFi.SSID(idx) + "=" + String(WiFi.RSSI(idx)) + ";";
        if(networks.length() > 160) {
            break;
        }
    }

    m_console->println("wifi=scanresult; // " + networks);

    return networks;
}

void WiFiConnectionHelper::loop()
{
    // If WiFi isn't enabled, to bother trying to connect.
    if (!m_sysConfig->WiFiEnabled)
    {
        return;
    }

    // If we aren't commissioned, then don't bother trying to connect.
    if (!m_sysConfig->Commissioned)
    {
        return;
    }

    int status = WiFi.status();

    // If we are connected, life is good, just mark us as connected and bail.
    if (status == WL_CONNECTED)
    {
        m_attempt = 0;

        if (m_wifiState != NuvIoTWiFi_Connected)
        {            
            // m_display->clearBuffer();
            // m_display->drawString(0, 0, "Connected to:");
            // m_display->drawString(80, 0, m_sysConfig->WiFiSSID.c_str());
            // m_display->sendBuffer();
            m_console->println("wifi=connected; // wifi state transitioned to connected.");

            IPAddress ip = WiFi.localIP();

            String sIPAddress = "";
            for (int i = 0; i < 4; i++)
                sIPAddress += i ? "." + String(ip[i]) : String(ip[i]);

            m_console->println("wifi_ipaddress=" + sIPAddress + ";");
            m_ledManager->setErrFlashRate(0);
            m_ledManager->setOnlineFlashRate(0);
            m_state->setWiFiState(WiFi_Connected);
            m_state->setWiFiIPAddress(sIPAddress);
            m_wifiState = NuvIoTWiFi_Connected;

            uint32_t freeHeep = ESP.getFreeHeap();
            m_console->println("[WiFiConnectionHelper__loop]: freeafterconnect= " + String(freeHeep));
        }

        // This is the state we normally want to be in.
        return;
    }

    if((millis() - m_lastReconnect) < 5000) {
        return;
    }

    if (m_wifiState == NuvIoTWiFi_Connected)
    {
        m_console->println("wifi=lostconnection; // Starting to reconnecting.");
        if(connect(true))
            m_attempt++;
        return;
    }

    m_ledManager->setErrFlashRate(10);
    m_ledManager->setOnlineFlashRate(10);

    if (m_wifiState == NuvIoTWiFi_NotConnected)
    {
        m_state->setWiFiState(WiFi_Disconnected);
        m_console->println("wifi=notconnected; // Starting to connect.");
        if(connect(false)) 
            m_attempt++;
    }
    else if(m_wifiState == NuvIoTWiFi_Connecting)
    {
        m_console->println("wifi=connecting; // attempt=" + String(m_attempt) + ", status=" + getWiFiStatus());
        if(connect(false))
            m_attempt++;
    }

    // If we are connected, then that means we have lost our connection.


    
    // m_display->clearBuffer();
    // m_display->drawString(0, 0, "NuvIoT");
    // m_display->drawString(0, 16, "Connecting WiFi");
    // m_display->drawString(0, 32, m_sysConfig->WiFiSSID.c_str());

    // if (m_spinnerIndex == 0)
    //     m_display->drawString(90, 0, "/");
    // else if (m_spinnerIndex == 1)
    //     m_display->drawString(90, 0, "-");
    // else if (m_spinnerIndex == 2)
    //     m_display->drawString(90, 0, "\\");

    // m_display->drawString(70, 0, String(m_attempt).c_str());

    m_spinnerIndex++;
    if (m_spinnerIndex == 3)
        m_spinnerIndex = 0;

    String statusMsg = getWiFiStatus();
    // m_display->drawString(0, 48, statusMsg);
    // m_display->sendBuffer();
    m_console->println("wifi=connecting; // attempt=" + String(m_attempt) + ", status=" + statusMsg);
    delay(250);

    m_state->setWiFiState(WiFi_Disconnected);
    m_state->setWiFiRSSI(0);
    m_state->setWiFiIPAddress("");
    m_state->setIsCloudConnected(false);

    if (m_attempt >= 10 && !m_state->getIsBleConnected())
    {
        m_hal->restart();
    }
}

bool WiFiConnectionHelper::connect(bool isReconnect){
    if((millis() - m_lastReconnect) < 5000) {
        return false;
    }

    if(!m_sysConfig->WiFiEnabled) {
        m_console->println("wifi=notenabled;");    
        return false;
    }

    if(m_sysConfig->WiFiSSID.length() == 0) {
        m_console->println("wifi=nossid; // WIFi enabled, attempt to connect but no SSID configured.");    
        return false;
    }

    m_lastReconnect = millis();

    m_isReconnect = isReconnect;
    m_wifiState = NuvIoTWiFi_Connecting;
    m_console->println("wifi=connect;");
    int status = WiFi.status();

    m_ledManager->setOnlineFlashRate(2);

    if (status == WL_CONNECTED)
    {
        m_console->printError("wifi=disconnectingprevious; // Disconnecting previous connection.");     
        WiFi.disconnect();
    }

    String statusMsg = "connecting";

    if (!WiFi.enableSTA(true))
    {
        statusMsg = "Fail enable STA";
        m_console->println("wifi=connecting; // attempt=" + String(m_attempt) + ", status=fail enable STA");
        // m_display->drawString(0, 48, "Fail enable STA");
        // m_display->sendBuffer();
        m_ledManager->setErrFlashRate(2);
        return true;
    }

    m_console->println("wifi=connecting; // ssid=" + m_sysConfig->WiFiSSID + ", pwd=" + m_sysConfig->WiFiPWD.c_str());
    wl_status_t beginResult = WiFi.begin(m_sysConfig->WiFiSSID.c_str(), m_sysConfig->WiFiPWD.c_str());
    
    m_ledManager->setErrFlashRate(6);

    return true;
}

int32_t WiFiConnectionHelper::getRSSI()
{
    return (int32_t)WiFi.RSSI();
}

bool WiFiConnectionHelper::isConnected()
{
    return m_wifiState == NuvIoTWiFi_Connected;
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
    m_wifiState = NuvIoTWiFi_NotConnected;
    m_client->stop();
}

void WiFiConnectionHelper::post(String addr, uint16_t port, String path, String body){
    HTTPClient client;

    if(client.begin(addr, port, path)) {
        if(client.connected()){
            m_console->println("Already connected: " + addr + ":" + String(port) + path);            
        }

        client.addHeader("Content-Type", "application/json");
        
        m_console->println("Web Request: " + addr + ":" + String(port) + path);

        int responseCode = client.POST(body);
        //if(responseCode == )
        client.end();

        m_console->println("HTTP Response: " + String(responseCode));
        
    } else {
        m_console->println("Could not connect to " + addr + " on port " + String(port));
    }
}

void WiFiConnectionHelper::setup()
{
    connect(false);
}