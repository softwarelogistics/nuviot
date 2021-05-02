#ifndef REST_H
#define REST_H

#include "SIMModem.h"
#include "Display.h"
#include "SysConfig.h"
#include "WiFiConnectionHelper.h"
#include "Console.h"
#include "NuvIoTClient.h"

class Rest {
    private:
        SIMModem *m_modem;
        NuvIoTClient *m_client;
        Console *m_console;
        WiFiConnectionHelper *m_wifiConnectionHelper;
        SysConfig *m_sysConfig;
        Display *m_display;
        String m_lastError = "OK";

    public: 
        Rest(NuvIoTClient *client, Display *display, SIMModem *modem, WiFiConnectionHelper *wifiConnectionHelper, SysConfig *sysConfig, Console *console) {
            m_modem = modem;
            m_wifiConnectionHelper = wifiConnectionHelper;
            m_console = console;
            m_display = display;
            m_client = client;
            m_sysConfig = sysConfig;
        };

        bool post(String url, String payload);
        bool get(String url);

        String getLastError();
};

#endif
