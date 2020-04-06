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
#define S_CONNECT "CONNECT"
#define S_CLOSED "CLOSED"
#define S_RESET "RESET!"

#define TEMP_BUFFER_SIZE 2048

#include <Arduino.h>
#include "Console.h"
#include "Channel.h"

class SIMModem {
public:
    SIMModem(Channel *channel, Console *console);

    bool isServiceConnected();
    bool isModemOnline();
    bool connectGPRS();
    bool connectServer(String hostName, String port);
    bool resetModem();
    bool enableErrorMessages();
    String getSIMId();
    String getNetwork();
    String getIPAddress();
    int getSignalQuality();

    bool beginDownload(String url);

    bool enableTransparentMode();
    bool disableTransparentMode();
    bool exitCommandMode();
    bool connect(String apn, String apnPwd, String apnUid);

private:
    byte m_tempBuffer[TEMP_BUFFER_SIZE];
    String m_lastError;
    String m_apn;
    String m_apnUid;
    String m_apnPwd;
    String m_simId;
    String m_network;
    String m_ipAddress;

    bool waitForReply(String expectedReply, int iterations);
  
    String sendCommand(String command);
    String sendCommand(String cmd, String expectedReply, unsigned long delayMS, long timeout, boolean returnAny);    bool setNBIoTMode();
    String parseIPAddress();
    
    bool setBearer();
    bool setLTE();
    bool setAPN();
    bool setBand();
    bool sisconnectIP();
    bool disconnectIP();
    bool getCREG();
    bool getCGREG();   
  
    Console *m_console;
    Channel *m_channel;

    int m_cmdIdx;
};

#endif
