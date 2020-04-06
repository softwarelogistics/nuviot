#ifndef OTASERVICES_H
#define OTASERVICES_H

#include "Display.h"
#include "Hal.h"
#include "SIMModem.h"

typedef enum
{
    DownloadingAndSaving_e = 0x01,
    Flashing_e
} DlState;

typedef struct
{
    char *url;
    char *caCert;
    char *md5;;
} DlInfo;

class OtaServices
{
private:
    Display *m_display;
    Hal *m_hal;
    SIMModem *m_modem;
    Console *m_console;

public:
    OtaServices(Display *display, Console *console, SIMModem *modem, Hal *hal);
    OtaServices(Display *display, Console *console, Hal *hal);
    ~OtaServices();

    bool downloadWithModem(String url);
    bool downloadOverWiFi(String url);

    bool start(String url);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HttpFOTA)
extern OtaServices httpFOTA;
#endif

#endif
