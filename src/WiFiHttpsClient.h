#ifndef NUVIOT_WIFI_HTTPS_CLIENT_H
#define NUVIOT_WIFI_HTTPS_CLIENT_H

#include <WiFiClientSecure.h>

class Console;
class Hal;

// ESP32 Wi-Fi TLS client using the ESP-IDF embedded root CA bundle.
// Certificate-chain, hostname and certificate-validity checks are mandatory.
class WiFiHttpsClient : public WiFiClientSecure
{
public:
    WiFiHttpsClient();

    static bool isDeviceClockValid();
    static bool synchronizeClock(Hal *hal, Console *console, unsigned long timeoutMs = 15000);

    int connect(const char *host, uint16_t port) override;
};

#endif
