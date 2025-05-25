#ifndef OTASERVICES_H
#define OTASERVICES_H

#ifdef LCD_DISPLAY
#include "Display.h"
#endif

#include "Hal.h"
#ifdef CELLULAR_ENABLED
#include "SIMModem.h"
#endif

#include <HTTPClient.h>
#include "NuvIoTState.h"

typedef enum
{
    DownloadingAndSaving_e = 0x01,
    Flashing_e
} DlState;

#define TEMP_BUFFER_SIZE 1024
#define RCV_BUFFER_SIZE 65535

typedef struct
{
    char *url;
    char *caCert;
    char *md5;;
} DlInfo;

class OtaServices
{
private:
    String m_url;
#ifdef LCD_DISPLAY
    Display *m_display;
#endif

    Hal *m_hal;
#ifdef CELLULAR_ENABLED
    SIMModem *m_modem;
#endif

    Console *m_console;
    NuvIoTState *m_state;

    uint8_t *m_recvBuffer = NULL;
    uint8_t *m_tempBuffer = NULL;
    
    int64_t getFileSize(HTTPClient *client, String url);
    int64_t applyBlock(HTTPClient *client, String url, uint32_t start, size_t contentSize);
    int64_t downloadContent(Stream *stream, size_t contentSize);
    void markCompleted(HTTPClient *client, String url, bool success);

public:
#ifdef LCD_DISPLAY
    OtaServices(Display *display, Console *console, NuvIoTState *state, SIMModem *modem, Hal *hal);
    OtaServices(Display *display, Console *console, NuvIoTState *state, Hal *hal);
 #endif

 #ifdef CELLULAR_ENABLED
    OtaServices(Console *console, NuvIoTState *state, SIMModem *modem, Hal *hal);
#endif    

    OtaServices(Console *console, NuvIoTState *state, Hal *hal);
    
    ~OtaServices();
#ifdef CELLULAR_ENABLED   
    bool downloadWithModem(String url);
    bool downloadWithModem();
#endif

    bool downloadOverWiFi();
    bool downloadOverWiFi(String url);
    void setDownloadUrl(String url) {m_url = url;}

    bool start(String url);
};

// #if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HttpFOTA)
// extern OtaServices httpFOTA;
// #endif

#endif
