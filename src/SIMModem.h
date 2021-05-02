#ifndef SIMMODEM_H
#define SIMMODEM_H

#define S_OK "OK"
#define S_ERROR "ERROR"
#define S_CALL_READY "Call Ready"
#define S_SMS_READY "SMS Ready"
#define S_TIMEOUT "TIMEOUT"
#define S_OK "OK"
#define S_AT "AT"
#define S_SHUT_OK "SHUT OK"
#define S_CLOSE_OK "CLOSE OK"
#define S_CONNECT "CONNECT"
#define S_CONNECT_OK "CONNECT OK"
#define S_CLOSED "CLOSED"
#define S_RESET "RESET!"
#define S_NOSIM "NOSIM"

#define TEMP_BUFFER_SIZE 2048
#define DOWNLOAD_BUFFER_SIZE 16384l

#include <Arduino.h>
#include "Hal.h"
#include "Console.h"
#include "Channel.h"
#include "Display.h"
#include "GPSData.h"

class SIMModem {
public:
    SIMModem(Display *display, Channel *channel, Console *console, Hal *hal);
    SIMModem(Channel *channel, Console *console, Hal *hal);

    bool isServiceConnected();
    bool isModemOnline();
    bool connectGPRS();
    bool disconnectGPRS();
    bool connectServer(String hostName, String port);
    bool resetModem();
    bool enableErrorMessages();
    String getSIMId();
    bool setBaudRate(unsigned long baudRate);
    String getNetwork();
    String getIPAddress();
    int getSignalQuality();    

    bool beginDownload(String url);
    bool init();
    bool enableTransparentMode();
    bool disableTransparentMode();
    bool exitDataMode();
    bool connect(String apn, String apnPwd, String apnUid);


    String httpGet(String url);
    String httpPost(String url, String payload);

    bool initGPS();
    void startGPS();
    void stopGPS();
    GPSData *readGPS();
    
private:
    byte m_tempBuffer[TEMP_BUFFER_SIZE];
    byte m_rxBuffer[DOWNLOAD_BUFFER_SIZE];

    String m_lastError;
    String m_apn;
    String m_apnUid;
    String m_apnPwd;
    String m_simId;
    String m_network;
    String m_ipAddress;
    bool m_isHttpSessionActive = false;
    int m_rssi;

    GPSData *m_gpsData;

    bool waitForReply(String expectedReply, int iterations);
  
    String sendCommand(String command);
    String sendCommand(String cmd, String expectedReply, unsigned long delayMS, long timeout, boolean returnAny);    bool setNBIoTMode();
    String parseIPAddress();
    
    uint32_t downloadContent(uint32_t contentSize, unsigned char * buffer);
    uint32_t configureForDownload(String url);
    bool setBearer();
    bool setLTE();
    bool selectNetwork();
    bool setAPN();
    bool setPDPContext();    
    bool setBand();
    bool sisconnectIP();
    bool disconnectIP();
    bool getCREG();
    bool getCGREG();   
    int findRSSI();

    bool setupHttpContext(String tag, String url);
    String readHttpResponse(String tag);
    void closeHttpContext(String tag);

    bool httpGetNoContent(String url);
    bool httpGetSetError(String url, String errMsg);
    
  
    Hal *m_hal;
    Display *m_display;
    Console *m_console;
    Channel *m_channel;

    int m_cmdIdx;
};

#endif
