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
#define S_CONNECT_OK "CONNECT OK"
#define S_RESET "RESET!"

#include <Arduino.h>
#include "Console.h"
#include "Channel.h"

class SIMModem {
public:
    SIMModem(Stream *stream, Console *console);

    bool isServiceConnected();
    bool isModemOnline();
    bool connectGPRS();
    bool resetModem();
    bool enableErrorMessages();
    String getSIMId();
    String getNetwork();
    String getIPAddress();
    int getSignalQuality();

    bool enableTransparentMode();
    bool enableCommandMode();
    bool connect(String apn, String apnPwd, String apnUid);

private:
    byte m_tempBuffer[2048];
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
