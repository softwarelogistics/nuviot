#ifndef OTASERVICES_H
#define OTASERVICES_H

#include "Display.h"
#include "Hal.h"
#include "SIMModem.h"
#include <HTTPClient.h>
#include "NuvIoTState.h"

typedef enum
{
    DownloadingAndSaving_e = 0x01,
    Flashing_e
} DlState;

#define TEMP_BUFFER_SIZE 2048
#define RCV_BUFFER_SIZE 8192


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
    Display *m_display;
    Hal *m_hal;
    SIMModem *m_modem;
    Console *m_console;
    NuvIoTState *m_state;

    uint8_t *m_recvBuffer = NULL;
    
    int64_t getFileSize(HTTPClient *client, String url);
    int64_t applyBlock(HTTPClient *client, String url, uint32_t start, uint16_t contentSize);
    int64_t downloadContent(Stream *stream, uint32_t contentSize);
    void markCompleted(HTTPClient *client, String url, bool success);

public:
    OtaServices(Display *display, Console *console, NuvIoTState *state, SIMModem *modem, Hal *hal);
    OtaServices(Display *display, Console *console, NuvIoTState *state, Hal *hal);
    ~OtaServices();

    bool downloadWithModem(String url);
    bool downloadWithModem();
    bool downloadOverWiFi();
    bool downloadOverWiFi(String url);
    void setDownloadUrl(String url) {m_url = url;}

    bool start(String url);
};

// #if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HttpFOTA)
// extern OtaServices httpFOTA;
// #endif

#endif
