#include "CellularRest.h"

#ifdef CELLULAR_ENABLED
bool CellularRest::get(String url) {
    m_console->println("GET: " + url) ;
    delay(500);
    
    if(m_client->connectToAPN(false, true, m_sysConfig->GPRSModemBaudRate)) {
        m_console->println("CONNECTED TO GPRS") ;
        delay(500);
        m_modem->httpGet(url);
    }
    else {
        m_console->println("COULD NOT CONNECTED TO GPRS") ;
        delay(500);
    }

    m_client->disconnectFromAPN();

    m_console->println("DISCONNECTED TO GPRS") ;
    delay(500);

    return true;
}

bool CellularRest::post(String url, String payload) {
    m_console->println("POST: " + url) ;
    m_console->println("DATA: " + payload) ;
    delay(500);
    
    if(m_client->connectToAPN(false, true, m_sysConfig->GPRSModemBaudRate)) {        
        m_console->println("CONNECTED TO GPRS") ;
        delay(500);

        m_modem->httpPost(url, payload);
    }
    else {
        m_console->println("COULD NOT CONNECTED TO GPRS") ;
        delay(500);
    }

    m_client->disconnectFromAPN();

    m_console->println("DISCONNECTED TO GPRS") ;
    delay(500);

    return true;
}
#endif

String CellularRest::getLastError() {
    return m_lastError;   
}