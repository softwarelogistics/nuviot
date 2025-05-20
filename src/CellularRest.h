#ifndef REST_H
#define REST_H
#include "SIMModem.h"
#include "SysConfig.h"
#include "WiFiConnectionHelper.h"
#include "Console.h"
#include "NuvIoTClient.h"

class CellularRest {
    private:
        SIMModem *m_modem;
  
        NuvIoTClient *m_client;
        Console *m_console;
        WiFiConnectionHelper *m_wifiConnectionHelper;
        SysConfig *m_sysConfig;
        String m_lastError = "OK";

    public: 
        CellularRest(NuvIoTClient *client, SIMModem *modem, WiFiConnectionHelper *wifiConnectionHelper, SysConfig *sysConfig, Console *console) {
            m_modem = modem;
            m_wifiConnectionHelper = wifiConnectionHelper;
            m_console = console;
            m_client = client;
            m_sysConfig = sysConfig;
        };  

        #ifdef CELLULAR_ENABLED
        bool post(String url, String payload);
        bool get(String url);
        #endif

        String getLastError();
};

#endif
