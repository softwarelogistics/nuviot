#ifndef OtaServices_h
#define OtaServices_h

#include "Display.h"
#include "Hal.h"

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
public:
    OtaServices(Display *display, Hal *hal);
    ~OtaServices();

    int start(String url);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HttpFOTA)
extern OtaServices httpFOTA;
#endif

#endif