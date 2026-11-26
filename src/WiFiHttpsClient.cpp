#include "WiFiHttpsClient.h"

#include "Console.h"
#include "Hal.h"

#include <time.h>
#include <stdint.h>
#include <mbedtls/x509_crt.h>

// ESP-IDF 4.4.4 embeds the generated full root bundle in libmbedtls.
// Reuse that exact bundle to initialize Arduino-ESP32 2.0.6's separate index.
extern "C" const uint8_t x509_crt_imported_bundle_bin_start[] asm("_binary_x509_crt_bundle_start");

#if defined(ARDUINO_ARCH_ESP32) && (!defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE) || !CONFIG_MBEDTLS_CERTIFICATE_BUNDLE)
#error "Wi-Fi HTTPS requires the ESP-IDF trusted certificate bundle"
#endif

namespace
{
    const time_t EarliestValidDeviceTime = static_cast<time_t>(1704067200); // 2024-01-01 UTC
    bool certificateBundleInitialized = false;
    bool certificateBundleReported = false;
    uint16_t certificateBundleCount = 0;

    int compareCertificateTime(const mbedtls_x509_time &certificateTime, const tm &currentTime)
    {
        const int certificateValues[] = {
            certificateTime.year, certificateTime.mon, certificateTime.day,
            certificateTime.hour, certificateTime.min, certificateTime.sec
        };
        const int currentValues[] = {
            currentTime.tm_year + 1900, currentTime.tm_mon + 1, currentTime.tm_mday,
            currentTime.tm_hour, currentTime.tm_min, currentTime.tm_sec
        };

        for (size_t i = 0; i < sizeof(certificateValues) / sizeof(certificateValues[0]); ++i)
        {
            if (certificateValues[i] < currentValues[i]) return -1;
            if (certificateValues[i] > currentValues[i]) return 1;
        }
        return 0;
    }

    bool isSaneCertificateTime(const mbedtls_x509_time &value)
    {
        return value.year >= 1970 && value.mon >= 1 && value.mon <= 12 &&
            value.day >= 1 && value.day <= 31 && value.hour >= 0 && value.hour <= 23 &&
            value.min >= 0 && value.min <= 59 && value.sec >= 0 && value.sec <= 60;
    }

    bool isPeerCertificateTimeValid(const mbedtls_x509_crt *certificate)
    {
        if (!certificate || !WiFiHttpsClient::isDeviceClockValid())
            return false;

        const time_t now = time(NULL);
        tm currentTime;
        if (!gmtime_r(&now, &currentTime))
            return false;

        // Check the full peer-supplied chain, not only its leaf certificate.
        for (const mbedtls_x509_crt *item = certificate; item; item = item->next)
        {
            if (!isSaneCertificateTime(item->valid_from) ||
                !isSaneCertificateTime(item->valid_to) ||
                compareCertificateTime(item->valid_from, currentTime) > 0 ||
                compareCertificateTime(item->valid_to, currentTime) < 0)
                return false;
        }
        return true;
    }
}

WiFiHttpsClient::WiFiHttpsClient()
{
    // Initialize Arduino-ESP32's separate bundle index from the full ESP-IDF
    // root bundle. The explicit diagnostic is emitted by synchronizeClock().
    initializeCertificateBundle(NULL);
    setHandshakeTimeout(10);
}

uint16_t WiFiHttpsClient::getEmbeddedBundleCertificateCount()
{
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(x509_crt_imported_bundle_bin_start[0]) << 8) |
        static_cast<uint16_t>(x509_crt_imported_bundle_bin_start[1]));
}

bool WiFiHttpsClient::initializeCertificateBundle(Console *console)
{
    if (!certificateBundleInitialized)
    {
        certificateBundleCount = getEmbeddedBundleCertificateCount();
        if (certificateBundleCount == 0)
        {
            if (console)
                console->printError(F("[HTTPS] ESP-IDF embedded CA bundle is empty"));
            return false;
        }

        if (console)
            console->println(String(F("[HTTPS] initializing Arduino CA index from ESP-IDF bundle: ")) +
                String(certificateBundleCount) + F(" certificates"));

        setCACertBundle(x509_crt_imported_bundle_bin_start);
        certificateBundleInitialized = true;
    }

    if (console && !certificateBundleReported)
    {
        console->println(String(F("[HTTPS] NuvIoT supplied ")) +
            String(certificateBundleCount) + F(" root certificates to Arduino TLS"));
        certificateBundleReported = true;
    }

    return certificateBundleInitialized;
}

bool WiFiHttpsClient::isDeviceClockValid()
{
    return time(NULL) >= EarliestValidDeviceTime;
}

bool WiFiHttpsClient::synchronizeClock(Hal *hal, Console *console, unsigned long timeoutMs)
{
    if (!initializeCertificateBundle(console))
        return false;

    if (isDeviceClockValid())
        return true;

    if (console)
        console->println(F("[HTTPS] device clock unset; synchronizing with NTP"));

    configTime(0, 0, "pool.ntp.org", "time.cloudflare.com", "time.google.com");
    const unsigned long started = millis();

    while (millis() - started < timeoutMs)
    {
        if (isDeviceClockValid())
        {
            if (console)
                console->println(F("[HTTPS] device clock synchronized"));
            return true;
        }

        if (hal)
            hal->feedHWWatchdog();
        delay(100);
    }

    if (console)
        console->printError(F("[HTTPS] valid device clock unavailable; refusing TLS"));

    return false;
}

int WiFiHttpsClient::connect(IPAddress, uint16_t)
{
    return 0;
}

int WiFiHttpsClient::connect(IPAddress, uint16_t, int32_t)
{
    // HTTPClient uses the three-argument virtual overload. Reject IP literals
    // because they do not provide a DNS hostname to validate against the cert.
    return 0;
}

int WiFiHttpsClient::connect(const char *host, uint16_t port)
{
    if (!isDeviceClockValid())
        return 0;

    const int result = WiFiClientSecure::connect(host, port);
    if (result <= 0)
        return result;

    if (!isPeerCertificateTimeValid(getPeerCertificate()))
    {
        stop();
        return 0;
    }

    return result;
}

int WiFiHttpsClient::connect(const char *host, uint16_t port, int32_t timeout)
{
    if (!isDeviceClockValid())
        return 0;

    // This is the overload HTTPClient calls. Base implementation performs
    // CA-chain and hostname validation; then enforce validity dates ourselves.
    const int result = WiFiClientSecure::connect(host, port, timeout);
    if (result <= 0)
        return result;

    if (!isPeerCertificateTimeValid(getPeerCertificate()))
    {
        stop();
        return 0;
    }

    return result;
}
