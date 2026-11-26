#include "SIMModem.h"
#include "Utils.h"
#include "OtaTransfer.h"
#include "CellularOtaTrust.h"
#include <string.h>
#include <Update.h>
#include <esp_spi_flash.h>
#include <esp_ota_ops.h>
#include <esp_image_format.h>

#ifdef LCD_DISPLAY
SIMModem::SIMModem(Display *display, Channel *channel, Console *console, Hal *hal, ConfigPins *configPins)
{
    m_hal = hal;
    m_channel = channel;
    m_console = console;
    m_display = display;
    m_configPins = configPins;
    m_gpsData = new GPSData();
}
#endif

SIMModem::SIMModem(Channel *channel, Console *console, Hal *hal, ConfigPins *configPins)
{
    m_hal = hal;
    m_channel = channel;
    m_console = console;
    #ifndef LCD_DISPLAY
    #endif
    m_configPins = configPins;
    m_gpsData = new GPSData();
}

bool SIMModem::waitForReply(String expectedReply, int iterations)
{
    bool shouldContinue = true;
    int loopCount = 0;
    while (shouldContinue)
    {
        int bytesToRead = m_channel->available();
        if (bytesToRead >= expectedReply.length())
        {
            m_channel->readBytes(m_tempBuffer, expectedReply.length());

            m_console->printByteArray("Original Message: ", m_tempBuffer, expectedReply.length());

            if (strncmp(expectedReply.c_str(), (char *)m_tempBuffer, expectedReply.length()) == 0)
            {
                m_console->printVerbose("WaitForReply - Match: [" + expectedReply + "] Iteration: " + String(loopCount));
                return true;
            }
        }

        loopCount++;
        if (iterations != -1 && loopCount > iterations)
        {
            m_console->printError("Timed out waiting for: [" + expectedReply + "] Iterations => " + String(iterations));
            return false;
        }

        delay(250);
    }

    return false;
}

bool SIMModem::enableTransparentMode()
{
    return sendCommand("AT+CIPMODE=1") == S_OK;
}

bool SIMModem::disableTransparentMode()
{
    //return sendCommand("AT+CIPMODE=0") == S_OK;

    return true;
}

bool SIMModem::exitDataMode()
{
    delay(1000);
    m_channel->print("+++");
    delay(1000);

    bool result = waitForReply("OK", 5);
    if (result)
    {
        m_console->println("Enable command mode.");
    }
    else
    {
        m_console->printError("Did not enable command mode.");
    }

    return result;
}

bool SIMModem::parseSecureUrl(String url, String &origin, String &host, String &path)
{
    if (!url.startsWith("https://") || url.length() > 512)
        return false;

    const int authorityStart = 8;
    int authorityEnd = url.length();
    const int pathStart = url.indexOf('/', authorityStart);
    const int queryStart = url.indexOf('?', authorityStart);
    const int fragmentStart = url.indexOf('#', authorityStart);
    if (fragmentStart >= 0)
        return false;
    if (pathStart >= 0 && pathStart < authorityEnd)
        authorityEnd = pathStart;
    if (queryStart >= 0 && queryStart < authorityEnd)
        authorityEnd = queryStart;

    const String authority = url.substring(authorityStart, authorityEnd);
    if (!authority.length() || authority.indexOf('@') >= 0 || authority.indexOf('"') >= 0)
        return false;

    const int portSeparator = authority.indexOf(':');
    host = portSeparator >= 0 ? authority.substring(0, portSeparator) : authority;
    if (!host.length() || host.length() > 63 || host.charAt(0) == '.' || host.charAt(host.length() - 1) == '.')
        return false;

    for (size_t i = 0; i < host.length(); ++i)
    {
        const char ch = host.charAt(i);
        const bool valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '.';
        if (!valid || (ch == '-' && (i == 0 || host.charAt(i - 1) == '.' || i == host.length() - 1)))
            return false;
    }

    if (portSeparator >= 0)
    {
        if (authority.indexOf(':', portSeparator + 1) >= 0)
            return false;
        uint32_t port = 0;
        if (!OtaTransfer::parseDecimal(authority.substring(portSeparator + 1).c_str(), 65535, port) || !port)
            return false;
    }

    origin = String("https://") + authority;
    if (origin.length() > 64)
        return false;

    if (pathStart >= 0)
        path = url.substring(pathStart);
    else if (queryStart >= 0)
        path = String("/") + url.substring(queryStart);
    else
        path = "/";

    if (!path.length() || path.length() > 512 || path.indexOf('"') >= 0 ||
        path.indexOf('\r') >= 0 || path.indexOf('\n') >= 0 || path.indexOf('#') >= 0)
        return false;

    return true;
}

bool SIMModem::isCellularModemClockValid()
{
    const String response = sendCommand("AT+CCLK?", "", 0, 5000, true);
    const int firstQuote = response.indexOf('"');
    const int lastQuote = response.indexOf('"', firstQuote + 1);
    if (firstQuote < 0 || lastQuote <= firstQuote)
        return false;

    const String value = response.substring(firstQuote + 1, lastQuote);
    if (value.length() < 17 || value.charAt(2) != '/' || value.charAt(5) != '/' ||
        value.charAt(8) != ',' || value.charAt(11) != ':' || value.charAt(14) != ':')
        return false;

    uint32_t year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (!OtaTransfer::parseDecimal(value.substring(0, 2).c_str(), 99, year) ||
        !OtaTransfer::parseDecimal(value.substring(3, 5).c_str(), 12, month) ||
        !OtaTransfer::parseDecimal(value.substring(6, 8).c_str(), 31, day) ||
        !OtaTransfer::parseDecimal(value.substring(9, 11).c_str(), 23, hour) ||
        !OtaTransfer::parseDecimal(value.substring(12, 14).c_str(), 59, minute) ||
        !OtaTransfer::parseDecimal(value.substring(15, 17).c_str(), 60, second))
        return false;

    return year >= 24 && month > 0 && day > 0;
}

bool SIMModem::synchronizeCellularModemClock()
{
    // Do not trust an old-but-plausible RTC value. Synchronize for each new
    // HTTPS session so certificate validity is evaluated against current time.
    m_console->println("[OTA HTTPS] synchronizing modem clock with NTP");
    if (sendCommand("AT+CNTPCID=1", S_OK, 0, 5000, false) != S_OK ||
        sendCommand("AT+CNTP=\"pool.ntp.org\",0,1,0", S_OK, 0, 5000, false) != S_OK)
    {
        m_console->printError("[OTA HTTPS] NTP configuration failed");
        return false;
    }

    m_channel->transmit("AT+CNTP\r\n");
    bool acknowledged = false;
    bool synchronized = false;
    for (int attempt = 0; attempt < 12 && !(acknowledged && synchronized); ++attempt)
    {
        m_hal->feedHWWatchdog();
        String line = m_channel->readStringUntil('\n', 5000);
        line.trim();
        if (line == S_OK)
        {
            acknowledged = true;
            continue;
        }
        if (line == S_ERROR || line.startsWith("+CME ERROR:"))
        {
            m_console->printError("[OTA HTTPS] NTP synchronization failed");
            return false;
        }
        if (line.startsWith("+CNTP:"))
        {
            String result = line.substring(6);
            result.trim();
            const int comma = result.indexOf(',');
            if (comma >= 0)
                result = result.substring(0, comma);
            uint32_t code = 0;
            if (!OtaTransfer::parseDecimal(result.c_str(), 999, code) || code != 1)
            {
                m_console->printError("[OTA HTTPS] NTP service did not provide time");
                return false;
            }
            synchronized = true;
        }
    }

    if (!acknowledged || !synchronized || !isCellularModemClockValid())
    {
        m_console->printError("[OTA HTTPS] valid modem clock unavailable; refusing TLS");
        return false;
    }

    m_console->println("[OTA HTTPS] modem clock synchronized");
    return true;
}

bool SIMModem::installCellularOtaTrustAnchor()
{
    const size_t certificateLength = strlen(CellularOtaTrustAnchorPem);
    if (!certificateLength)
    {
        m_console->printError("[OTA HTTPS] trusted CA is not configured; refusing TLS");
        return false;
    }
    if (certificateLength > 8192)
    {
        m_console->printError("[OTA HTTPS] trusted CA exceeds modem file limit");
        return false;
    }
    if (m_isCellularOtaCaReady)
        return true;

    if (sendCommand("AT+CFSINIT") != S_OK)
    {
        m_console->printError("[OTA HTTPS] modem certificate storage unavailable");
        return false;
    }

    // Replace only our own fixed trust-anchor file. No customer or modem
    // credentials are read, changed, or logged here.
    sendCommand(String("AT+CFSDFILE=3,\"") + CellularOtaTrustAnchorFile + "\"");
    const String writeCommand = String("AT+CFSWFILE=3,\"") + CellularOtaTrustAnchorFile +
        "\",0," + String(static_cast<unsigned long>(certificateLength)) + ",1000";
    bool stored = sendCommand(writeCommand, "DOWNLOAD", 0, 10000, false) == S_OK;
    if (stored)
    {
        m_channel->transmit(reinterpret_cast<byte *>(const_cast<char *>(CellularOtaTrustAnchorPem)), certificateLength);
        stored = false;
        for (int attempt = 0; attempt < 6; ++attempt)
        {
            m_hal->feedHWWatchdog();
            String line = m_channel->readStringUntil('\n', 10000);
            line.trim();
            if (line == S_OK)
            {
                stored = true;
                break;
            }
            if (line == S_ERROR || line.startsWith("+CME ERROR:"))
                break;
        }
    }

    const bool fileSystemClosed = sendCommand("AT+CFSTERM") == S_OK;
    if (!stored || !fileSystemClosed)
    {
        m_console->printError("[OTA HTTPS] modem CA provisioning failed");
        return false;
    }

    if (sendCommand(String("AT+CSSLCFG=\"convert\",2,\"") + CellularOtaTrustAnchorFile + "\"") != S_OK)
    {
        m_console->printError("[OTA HTTPS] modem rejected trusted CA");
        return false;
    }

    m_isCellularOtaCaReady = true;
    m_console->println("[OTA HTTPS] Software Logistics trust anchor provisioned");
    return true;
}

bool SIMModem::configureSecureHttpContext(String url)
{
    String origin, host, path;
    if (!parseSecureUrl(url, origin, host, path))
    {
        m_console->printError("[OTA HTTPS] invalid HTTPS URL");
        return false;
    }

    if (m_isHttpSessionActive)
        return m_isSecureHttpSession && origin == m_secureOrigin && host == m_secureHost;

    if (!installCellularOtaTrustAnchor() || !synchronizeCellularModemClock())
        return false;

    if (sendCommand("AT+CSSLCFG=\"sslversion\",1,3") != S_OK ||
        sendCommand("AT+CSSLCFG=\"ignorertctime\",1,0") != S_OK ||
        sendCommand(String("AT+CSSLCFG=\"sni\",1,\"") + host + "\"") != S_OK ||
        sendCommand(String("AT+SHSSL=1,\"") + CellularOtaTrustAnchorFile + "\"") != S_OK ||
        sendCommand(String("AT+SHCONF=\"URL\",\"") + origin + "\"") != S_OK ||
        sendCommand("AT+SHCONF=\"BODYLEN\",1024") != S_OK ||
        sendCommand("AT+SHCONF=\"HEADERLEN\",350") != S_OK)
    {
        m_console->printError("[OTA HTTPS] modem TLS configuration failed");
        return false;
    }

    if (sendCommand("AT+SHCONN", S_OK, 0, 60000, false) != S_OK)
    {
        // SIMCom reports certificate/hostname/time failures through the TLS
        // connection result. Never retry the same request over HTTP.
        sendCommand("AT+SHDISC");
        m_console->printError("[OTA HTTPS] TLS connection rejected (CA, hostname or clock)");
        return false;
    }

    m_isHttpSessionActive = true;
    m_isSecureHttpSession = true;
    m_secureOrigin = origin;
    m_secureHost = host;
    m_console->println(String("[OTA HTTPS] TLS connected; host=") + host);
    return true;
}

int64_t SIMModem::configureSecureForDownload(String url, size_t maximum, bool readBody, bool exactLength)
{
    String origin, host, path;
    if (!parseSecureUrl(url, origin, host, path) || !configureSecureHttpContext(url))
        return -1;

    // SIMCom requires the absolute URL here, even though SHCONF has already
    // established the HTTPS origin. Keep this command off console logging:
    // the path contains the request-ID capability.
    const String command = String("AT+SHREQ=\"") + url + "\",1";
    m_channel->transmit(command + "\r\n");

    bool acknowledged = false;
    bool received = false;
    uint32_t status = 0;
    uint32_t length = 0;
    for (int attempt = 0; attempt < 14 && !(acknowledged && received); ++attempt)
    {
        m_hal->feedHWWatchdog();
        String line = m_channel->readStringUntil('\n', 5000);
        line.trim();
        if (line == command)
            continue;
        if (line == S_OK)
        {
            acknowledged = true;
            continue;
        }
        if (line == S_ERROR || line.startsWith("+CME ERROR:"))
            return -1;
        if (!line.startsWith("+SHREQ:"))
            continue;

        const int first = line.indexOf(',');
        const int second = first < 0 ? -1 : line.indexOf(',', first + 1);
        if (first < 0 || second < 0 || line.substring(8, first) != "\"GET\"" ||
            !OtaTransfer::parseDecimal(line.substring(first + 1, second).c_str(), 999, status) ||
            !OtaTransfer::parseDecimal(line.substring(second + 1).c_str(), INT32_MAX, length))
            return -1;
        received = true;
    }

    if (!acknowledged || !received)
        return -1;
    if ((readBody && status != 200) || (!readBody && (status < 200 || status >= 300)))
    {
        m_console->printWarning(String("[OTA HTTPS] HTTP status rejected: ") + String(status));
        return -1;
    }
    if (!readBody)
        return length;
    if (!length || length > maximum || (exactLength && length != maximum))
        return -1;

    return length;
}

int64_t SIMModem::readSecureHttpContent(uint32_t contentSize, unsigned char *buffer)
{
    if (!buffer || !contentSize || contentSize > DOWNLOAD_BUFFER_SIZE)
        return -1;

    uint32_t offset = 0;
    while (offset < contentSize)
    {
        size_t length = contentSize - offset;
        if (length > 2048)
            length = 2048;

        const String command = String("AT+SHREAD=") + String(offset) + "," + String(static_cast<unsigned long>(length));
        m_channel->transmit(command + "\r\n");

        bool acknowledged = false;
        bool received = false;
        for (int attempt = 0; attempt < 8 && !(acknowledged && received); ++attempt)
        {
            m_hal->feedHWWatchdog();
            String line = m_channel->readStringUntil('\n', 5000);
            line.trim();
            if (line == command)
                continue;
            if (line == S_OK)
            {
                acknowledged = true;
                continue;
            }
            if (line == S_ERROR || line.startsWith("+CME ERROR:"))
                return -1;
            if (!line.startsWith("+SHREAD:"))
                continue;

            uint32_t actual = 0;
            if (!acknowledged ||
                !OtaTransfer::parseDecimal(line.substring(8).c_str(), 2048, actual) ||
                actual != length)
                return -1;
            received = true;
        }

        if (!acknowledged || !received)
            return -1;

        const size_t actual = m_channel->readBytes(buffer + offset, length);
        // SHREAD data is binary. Never resume from a partial or uncertain read.
        if (actual != length)
            return -1;
        offset += static_cast<uint32_t>(actual);
    }

    return offset;
}

int64_t SIMModem::configureForDownload(String url, size_t maximum, bool readBody, bool exactLength)
{
    if (url.startsWith("https://"))
        return configureSecureForDownload(url, maximum, readBody, exactLength);
    if (!url.startsWith("http://") || m_isSecureHttpSession)
        return -1;

    if (!m_isHttpSessionActive)
    {
        for (int attempt = 0; attempt < 5 && !m_isHttpSessionActive; ++attempt)
        {
            if (sendCommand("AT+HTTPINIT") == S_OK)
                m_isHttpSessionActive = true;
            else
            {
                delay(1000);
                m_channel->clearBuffers();
            }
        }
        if (!m_isHttpSessionActive || sendCommand("AT+HTTPPARA=\"CID\",1") != S_OK)
            return -1;
    }

    if (sendCommand("AT+HTTPPARA=\"URL\",\"" + url + "\"") != S_OK)
        return -1;

    // Consume the command acknowledgement and asynchronous result here, not
    // through sendCommand(), which can otherwise swallow an early URC.
    m_channel->transmit("AT+HTTPACTION=0\r\n");
    uint32_t length = 0;
    bool acknowledged = false;
    bool received = false;
    for (int attempt = 0; attempt < 8 && !(acknowledged && received); ++attempt)
    {
        m_hal->feedHWWatchdog();
        String msg = m_channel->readStringUntil('\n', 5000);
        msg.trim();
        if (msg == S_ERROR) return -1;
        if (msg == S_OK) { acknowledged = true; continue; }
        if (!msg.startsWith("+HTTPACTION:")) continue;
        int first = msg.indexOf(',');
        if (first < 0) return -1;
        int second = msg.indexOf(',', first + 1);
        if (second < 0) return -1;
        uint32_t method = 0, status = 0;
        if (!OtaTransfer::parseDecimal(msg.substring(12, first).c_str(), 2, method) ||
            method != 0 ||
            !OtaTransfer::parseDecimal(msg.substring(first + 1, second).c_str(), 999, status) ||
            (readBody ? status != 200 : (status < 200 || status >= 300)) ||
            !OtaTransfer::parseDecimal(msg.substring(second + 1).c_str(), INT32_MAX, length))
            return -1;
        received = true;
    }
    if (!acknowledged || !received) return -1;
    if (!readBody) return length;
    if (!length || length > maximum || (exactLength && length != maximum)) return -1;

    m_channel->transmit("AT+HTTPREAD\r\n");
    for (int attempt = 0; attempt < 4; ++attempt)
    {
        String line = m_channel->readStringUntil('\n', 5000);
        line.trim();
        if (line == "ERROR") return -1;
        if (!line.startsWith("+HTTPREAD:")) continue;
        uint32_t actual = 0;
        if (!OtaTransfer::parseDecimal(line.substring(10).c_str(), INT32_MAX, actual) || actual != length)
            return -1;
        return length;
    }
    return -1;
}

bool SIMModem::closeHttpContext(String tag)
{
    if (m_isSecureHttpSession)
    {
        int retryCount = 0;
        bool done = false;
        while (retryCount++ < 5 && !done)
        {
            const String response = sendCommand("AT+SHDISC");
            if (response != S_OK)
            {
                m_console->printWarning("[OTA HTTPS] modem HTTPS disconnect failed");
                delay(500);
                m_channel->clearBuffers();
            }
            else
            {
                done = true;
            }
        }

        m_isHttpSessionActive = !done;
        if (done)
        {
            m_isSecureHttpSession = false;
            m_secureOrigin = "";
            m_secureHost = "";
        }
        return done;
    }

    int retryCount = 0;
    bool done = false;
    while (retryCount++ < 5 && done == false)
    {
        String response = sendCommand("AT+HTTPTERM");
        if (response != S_OK)
        {
            m_console->printWarning("fwupdate=nohttpterm; // Unexpected response [" + response + "] - area: " + tag + ".");
            delay(500);
            m_channel->clearBuffers();
        }
        else
        {
            done = true;
        }
    }

    m_isHttpSessionActive = !done;
    return done;
}

bool SIMModem::setupHttpContext(String tag, String url)
{
    // The legacy HTTP* helper is deliberately HTTP-only. HTTPS OTA uses the
    // certificate-validated SH* path and must never reach this adapter.
    if (!url.startsWith("http://"))
    {
        m_console->printError("http" + tag + "=unsupported_scheme;");
        return false;
    }

    m_console->println("http" + tag + "=start; // URL: " + url);

    int retryCount = 0;
    String response;
    bool done = false;
    while (retryCount++ < 5 && done == false)
    {
        response = sendCommand("AT+HTTPINIT");
        if (response != S_OK)
        {
            m_console->printWarning("http" + tag + "=unpexpectedresponse; // " + String(retryCount) + " failed http init err: " + response);
            delay(1000);
            m_channel->clearBuffers();
            return false;
        }
        else
        {
            done = true;
            m_isHttpSessionActive = true;
        }
    }

    if (!done)
    {
        m_console->printError("http" + tag + "=failed; // could not init http service, err: " + response);
    }

    response = sendCommand("AT+HTTPPARA=\"CID\",1");
    if (response != S_OK)
    {
        m_console->printError("http" + tag + "=failed; // failed set http param, cid=1, err: " + response);
        return "";
    }

    String httpParam = "AT+HTTPPARA=\"URL\",\"" + url + "\"";
    response = sendCommand(httpParam);
    if (response != S_OK)
    {
        m_console->printError("http" + tag + "=failed; // could not set url " + url + " err: " + response);
        return false;
    }

    return true;
}

String SIMModem::readHttpResponse(String tag)
{
    int retryCount = 0;
    bool done = false;

    String msg = "";
    while (!done && retryCount++ < 5)
    {
        msg = m_channel->readStringUntil('\n', 5000);
        msg.trim();

        if (msg != "")
        {
            if (msg.startsWith("+HTTPACTION:"))
            {
                int first = msg.indexOf(" ");
                int second = msg.indexOf(",", first);
                int third = msg.indexOf(",", second + 1);

                String actionType = msg.substring(first + 1, second);
                String httpResponseCode = msg.substring(second + 1, third);
                if (!httpResponseCode.startsWith("2"))
                {
                    m_console->printError("http" + tag + "=failed; // Non-Success HTTP Response Code: " + httpResponseCode);
                }
                String msgLen = msg.substring(third + 1);
                m_console->printVerbose(msg + " - Action Type=" + actionType + ", HTTP Result Code: " + httpResponseCode + ", Response Length: " + msgLen + ";");
                done = true;
            }
        }

        delay(retryCount * 500);
    }

    if (!done)
    {
        m_console->printError("http" + tag + "=failed; // could not begin download");
        return "";
    }

    m_channel->transmit("AT+HTTPREAD\r\n");
    String echoResponse = m_channel->readStringUntil('\n', 10000);
    echoResponse.trim();

    String readResponse = m_channel->readStringUntil('\n', 10000);
    readResponse.trim();

    String lenStr = readResponse.substring(11);

    memset(m_rxBuffer, 0, RX_BUFFER_SIZE);

    long contentLen = atol(lenStr.c_str());
    m_channel->readBytes(m_rxBuffer, contentLen);

    String content = String((char *)m_rxBuffer);
    m_console->println("http" + tag + "=succuss; // Length: " + lenStr + ", Content Response: " + content);

    m_channel->readStringUntil('\n', 5000);
    String finalOK = m_channel->readStringUntil('\n', 5000);
    return readResponse;
}

String SIMModem::httpGet(String url)
{
    if (!setupHttpContext("get", url))
    {
        m_console->printError("httpget=failed; // could not setup http context.");
        return "";
    }

    String response = sendCommand("AT+HTTPACTION=0");
    if (response != S_OK)
    {
        m_console->printError("httpget=failed; // could not set http action, err: " + response);
        return "";
    }

    response = readHttpResponse("get");

    closeHttpContext("get");

    return response;
}

String SIMModem::httpPost(String url, String payload)
{
    if (!setupHttpContext("post", url))
    {
        m_console->printError("httppost=failed; // could not setup http context.");
        return "";
    }

    String response = sendCommand("AT+HTTPDATA=" + String(payload.length()) + ",10000", "DOWNLOAD", 0, 500, false);
    if (response != S_OK)
    {
        m_console->printError("httppost=failed; // could set http data, err: " + response);
        return "";
    }

    delay(1000);

    m_channel->enqueueString(payload);
    m_channel->flush();

    if (!waitForReply("OK", 100))
    {
        m_console->printError("httppost=failed; // error waiting for OK after post upload");
        return "";
    }

    response = sendCommand("AT+HTTPACTION=1");
    if (response != S_OK)
    {
        m_console->printError("httppost=failed; // could not set http action, err: " + response);
        return "";
    }

    m_console->println("httppost=uploadedpayload; // Length: " + String(payload.length()) + ", Payload: " + payload);

    response = readHttpResponse("post");

    closeHttpContext("post");

    return response;
}

int64_t SIMModem::downloadContent(uint32_t contentSize, unsigned char *buffer)
{
    if (m_isSecureHttpSession)
        return readSecureHttpContent(contentSize, buffer);

    if (!buffer || !contentSize || contentSize > DOWNLOAD_BUFFER_SIZE) return -1;
    uint32_t offset = 0;
    while (offset < contentSize)
    {
        size_t length = contentSize - offset;
        if (length > 2048) length = 2048;
        m_hal->feedHWWatchdog();
        const size_t actual = m_channel->readBytes(buffer + offset, length);
        // Channel returns size_t(-1) and may consume partial bytes on timeout.
        // Discard the entire block; never resume from an uncertain offset.
        if (actual != length) return -1;
        offset += static_cast<uint32_t>(actual);
    }
    for (int attempt = 0; attempt < 4; ++attempt)
    {
        String line = m_channel->readStringUntil('\n', 1000);
        line.trim();
        if (line == S_OK) return offset;
        if (line == S_ERROR) return -1;
    }
    return -1;
}

bool SIMModem::httpGetNoContent(String url)
{
    const int64_t length = configureForDownload(url, 0, false);
    const bool closed = !m_isHttpSessionActive || closeHttpContext("get");
    return length >= 0 && closed;
}

String SIMModem::httpGetSetError(String url, String errorMsg)
{
    errorMsg.replace(" ", "_");
    errorMsg.replace(".", "_");
    errorMsg.replace(",", "_");

    return httpGet(url + "/failed?err=" + errorMsg);
}

#define MAX_CHUNK_DOWNLOAD_TRIES 5

bool SIMModem::DisconnectMQTT()
{
    uint8_t disconnectMessage[] = {
        0xE0,
        0x00};

    m_channel->enqueueByteArray(disconnectMessage, 2);

    m_console->printVerbose("NOT TRANSPARENT MODE - CIPSEND");

    uint16_t enqueuedBytes = m_channel->getEnqueuedLength();
    String sendMessage = "AT+CIPSEND=" + String(enqueuedBytes);
    m_channel->println(sendMessage);

    String response = m_channel->readStringUntil('\n', 3000);

    m_console->printVerbose("AT+CIPSEND= response: " + response);

    uint8_t ch = 0x00;
    uint16_t retryCount = 0;
    while (ch != '>' && retryCount++ < 500)
    {
        while (m_channel->available() > 0 && ch != '>')
        {
            ch = m_channel->readByte();
            m_console->printVerbose("UNEXPECTED RESPONSE: [" + String(ch) + "]");
        }
        delay(1);
    }

    if (ch != '>')
    {
        m_console->printError("mqttflush=fail; //timeout waiting for > ");
        return false;
    }

    m_console->printVerbose("RECEIVED: [" + String(ch) + "] Will continue");

    if (!m_channel->flush())
    {
        m_console->printError("mqttdisconnect=false;");
        return false;
    }
    else
    {
        m_console->println("mqttdisconnect=true;");
        delay(1000);
        return true;
    }
}

bool SIMModem::beginDownload(String url)
{
    if (!url.length() || (!url.startsWith("http://") && !url.startsWith("https://")))
    {
        m_console->printError("[OTA] unsupported firmware URL scheme");
        return false;
    }

    if (m_isHttpSessionActive && !closeHttpContext("previous"))
    {
        m_console->printError("[OTA] previous modem HTTP session could not be closed");
        return false;
    }

    if (url.startsWith("https://"))
    {
        String origin, host, path;
        if (!parseSecureUrl(url, origin, host, path))
        {
            m_console->printError("[OTA HTTPS] invalid HTTPS URL");
            return false;
        }
        m_console->println(String("[OTA HTTPS] modem=") + getDeviceModel() +
            " revision=" + getDeviceRevision() + " host=" + host);
    }

    alloc();
    bool success = false;
    bool begun = false;
    bool bearerReady = false;
    const char *failure = "modem_unavailable";
    m_hal->enableHWWatchdog(120);
    m_hal->feedHWWatchdog();

    if (m_rxBuffer && m_tempBuffer)
    {
        DisconnectMQTT();
        disconnectServer();
        delay(1000);
        m_channel->clearBuffers();
        const unsigned long start = millis();
        while (!isServiceConnected() && millis() - start < 60000UL)
        {
            m_hal->feedHWWatchdog();
            delay(500);
        }
        bearerReady = isServiceConnected() && setBearer();
    }
    if (bearerReady)
    {
        failure = url.startsWith("https://") ? "https_size_request_failed" : "invalid_firmware_size";
        const int64_t bodySize = configureForDownload(url + "/size", 32);
        if (bodySize > 0)
            failure = "invalid_firmware_size";
        uint32_t total = 0;
        bool sizeValid = false;
        if (bodySize > 0 && downloadContent(static_cast<uint32_t>(bodySize), m_rxBuffer) == bodySize)
        {
            m_rxBuffer[bodySize] = 0;
            sizeValid = !memchr(m_rxBuffer, 0, bodySize) &&
                OtaTransfer::parseDecimal(reinterpret_cast<const char *>(m_rxBuffer), INT32_MAX, total) && total != 0;
        }
        if (!sizeValid && m_isHttpSessionActive) closeHttpContext("size");
        const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
        if (sizeValid && partition && total <= partition->size)
        {
            failure = "update_begin_failed";
            begun = Update.begin(total, U_FLASH);
            if (begun)
            {
                bool contextUsable = true;
                const auto result = OtaTransfer::transfer(total, m_rxBuffer, DOWNLOAD_BUFFER_SIZE,
                    [&](uint32_t offset, uint8_t *buffer, size_t length) -> int64_t {
                        if (!contextUsable) return -1;
                        m_channel->clearBuffers();
                        String query = url + "?start=" + String(offset) + "&length=" + String(length);
                        const int64_t declared = configureForDownload(query, length, true, true);
                        int64_t actual = -1;
                        if (declared == static_cast<int64_t>(length))
                            actual = downloadContent(static_cast<uint32_t>(length), buffer);
                        if (actual != static_cast<int64_t>(length) && m_isHttpSessionActive)
                        {
                            m_channel->clearBuffers();
                            contextUsable = closeHttpContext("block");
                        }
                        return actual;
                    },
                    [](uint8_t *buffer, size_t length) { return Update.write(buffer, length); },
                    [&]() { m_hal->feedHWWatchdog(); });
                m_hal->feedHWWatchdog();
                failure = result == OtaTransfer::ReadFailed ? "chunk_download_failed" :
                    result == OtaTransfer::WriteFailed ? "flash_write_failed" : "flash_finalize_failed";
                success = result == OtaTransfer::Complete && Update.isFinished() && Update.end();
            }
        }
    }
    if (!success && begun) Update.abort();
    if (!success && m_isHttpSessionActive) m_channel->clearBuffers();
    const bool contextClosed = !m_isHttpSessionActive || closeHttpContext("ota");
    m_hal->feedHWWatchdog();
    if (bearerReady && contextClosed)
    {
        const String terminal = success ? String("/success") : String("/failed?err=") + failure;
        const bool reported = httpGetNoContent(url + terminal);
        if (!reported) m_console->printWarning("fwupdate=terminal_report_failed;");
    }
    else
        m_console->printWarning("fwupdate=terminal_report_unavailable;");
    if (success) m_console->println("fwupdate=success;");
    else m_console->printError(String("fwupdate=failed; // ") + failure);
#ifdef LCD_DISPLAY
    m_display->drawStr(success ? "Success flashing" : "Flashing failed", "Restarting");
#endif
    m_hal->feedHWWatchdog();
    m_hal->restart(2000);
    return success;
}

void SIMModem::hardwareReset()
{
    m_isHttpSessionActive = false;
    m_isSecureHttpSession = false;
    m_isCellularOtaCaReady = false;
    m_secureOrigin = "";
    m_secureHost = "";
    m_console->println("modem=starthardwareset; // Start Hardware Reset.");

    if (isModemOnline())
    {
        softwarePowerOff();
    }

    int retryCount = 0;
    while (isModemOnline() && retryCount++ < 5)
    {
        delay(1000);
    }

    if (isModemOnline())
    {
        m_console->printError("modem=swpwrofffail; // Software Power Off Failed, trying Hardware Power Off.");
        hardwarePowerOff();

        while (isModemOnline() && retryCount++ < 5)
        {
            delay(500);
        }

        if (isModemOnline())
        {
            m_console->printError("modem=hwpwrofffail; // Hardware Power Off failed, resetting.");
            delay(1000);
            m_hal->restart();
        }
    }
    else
    {
        delay(2000);
        m_console->println("modem=poweroff; // Modem is powered down.");
        hardwarePowerOn();

        while (!isModemOnline() && retryCount++ < 5)
        {
            delay(500);
        }

        if (!isModemOnline())
        {
            m_console->printError("modem=hwpwronfail; // Hardware Power on failed, resetting.");
            m_hal->restart();
        }
        else
        {
            m_console->println("modem=poweron; // Performed hardware reset of modem.");
        }
    }
}

String SIMModem::getDeviceModel()
{
    return sendCommand("AT+CGMM", "", 0, 1250, true);
}

String SIMModem::getDeviceRevision()
{
    return sendCommand("AT+CGMR", "", 0, 1250, true);
}

bool SIMModem::hardwarePowerOn()
{
    if (m_configPins->ModemResetPin != -1)
    {
        m_console->println("modem=poweron; // pin: " + String(m_configPins->ModemResetPin) + " set to low");

        pinMode(m_configPins->ModemResetPin, OUTPUT);
        digitalWrite(m_configPins->ModemResetPin, HIGH);
        delay(1600);
        digitalWrite(m_configPins->ModemResetPin, LOW);
        delay(500);
        digitalWrite(m_configPins->ModemResetPin, HIGH);

        m_console->println("modem=poweron; // pin: " + String(m_configPins->ModemResetPin) + " back high.");
        return true;
    }
    else
    {
        m_console->println("modem=poweron; // Power pin not configured.");
        return false;
    }
}

bool SIMModem::hardwarePowerOff()
{
    if (m_configPins->ModemResetPin != -1)
    {
        m_console->println("modem=poweroff; // pin: " + String(m_configPins->ModemResetPin) + " set to low.");

        pinMode(m_configPins->ModemResetPin, OUTPUT);
        digitalWrite(m_configPins->ModemResetPin, HIGH);
        delay(1600);
        digitalWrite(m_configPins->ModemResetPin, LOW);
        delay(1600);
        digitalWrite(m_configPins->ModemResetPin, HIGH);

        m_console->println("modem=poweroff; // pin: " + String(m_configPins->ModemResetPin) + " back to high.");
        return true;
    }
    else
    {
        m_console->println("modem=poweroff; // Power pin not configured.");
        return false;
    }
}

bool SIMModem::softwarePowerOff()
{
    if (isModemOnline())
    {
        sendCommand("AT+CPOWD=1");
        return true;
    }
    else
    {
        return false;
    }
}

String SIMModem::sendCommand(String cmd)
{
    return sendCommand(cmd, "", 0, 500, false);
}

String SIMModem::sendCommand(String cmd, String expectedReply, unsigned long delayMS, long timeout, boolean returnAny)
{
    int start = millis();
    String returnValue = S_OK;
    const bool sensitiveUrl = cmd.startsWith("AT+HTTPPARA=\"URL\"");
    if (!sensitiveUrl) m_console->printVerbose("sendcommand=sending; cmd=" + cmd);
    
    m_channel->transmit(cmd + "\r\n");
    delay(100);
    if (delayMS > 0)
    {
        delay(delayMS);
    }

    boolean shouldContinue = true;
    while (shouldContinue)
    {
        if (millis() - start > timeout)
        {
            if (sensitiveUrl) m_console->printWarning("sendcommand=warning; // URL command");
            else m_console->printWarning("sendcommand=warning; // Command " + cmd);
            return "-1";
        }

        if (m_channel->available() > 0)
        {
            String msg = m_channel->readStringUntil('\n', timeout);
            msg.trim();

            if (sensitiveUrl && msg == cmd) continue;
            if (!sensitiveUrl) m_console->printVerbose("sendcommand=response; msg=" + msg);

            if (msg.length() > 0)
            {
                if (msg == expectedReply)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " [" + msg + "] - ok");
                    m_cmdIdx++;
                    return S_OK;
                }
                else if (msg == S_OK && expectedReply == "")
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Will return value of [" + returnValue + "] - ok");
                    m_cmdIdx++;
                    return returnValue;
                }
                else if (msg == S_ERROR)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " [" + msg + "] - error");
                    return S_ERROR;
                }
                else if (msg == "+CPIN: NOT INSERTED")
                {
                    return S_NOSIM;
                }
                else if (msg == cmd)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Returned original -> " + msg);
                }
                else if (msg == S_RDY)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Received -> " + msg);
                }
                else if (msg == S_CPIN_READY)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Received -> " + msg);
                }
                else if (msg == S_SMS_READY)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Returned -> " + msg);
                    receivedSmsReady = true;
                }
                else if (msg == S_CALL_READY)
                {
                    m_console->printVerbose(String(m_cmdIdx) + " Returned -> " + msg);
                    receivedCallReady = true;
                }
                else
                {
                    if (returnAny)
                    {
                        m_console->printVerbose(String(m_cmdIdx) + " Returning result [" + msg + "]");
                        return msg;
                    }
                    else
                    {
                        if (!m_tempBuffer || msg.length() >= TEMP_BUFFER_SIZE) return S_ERROR;
                        if (sensitiveUrl) continue;
                        msg.getBytes(m_tempBuffer, msg.length());
                        if (isString(m_tempBuffer, msg.length()))
                        {
                            returnValue = msg;
                            m_console->printVerbose(String(m_cmdIdx) + " pending return value [" + msg + "]");
                        }
                        else
                        {
                            returnValue = "";
                            m_console->printByteArray("Binary Response - Length: " + String(msg.length()) + " Values: ", m_tempBuffer, msg.length());
                        }
                    }
                }
            }
        }
        else
        {
            delay(1);
            //m_console->printVerbose("sendcommand=waiting; expecting response: " + expectedReply);
            //delay(50);

        }
    }

    return S_ERROR;
}

String SIMModem::getNetwork()
{
    String response = sendCommand("AT+COPS?");
    if (response.length() > 12)
    {
        return response.substring(12, response.length() - 1);
    }
    else
    {
        return "??";
    }
}

bool SIMModem::enableErrorMessages()
{
    return sendCommand("AT+CMEE=2") == S_OK;
}

bool SIMModem::connectGPRS()
{
    return sendCommand("AT+CIICR", "", 0, 85000, false) == S_OK;
}

bool SIMModem::disconnectGPRS()
{
    return sendCommand("AT+CIPSHUT", S_SHUT_OK, 0, 85000, false) == S_OK;
}

bool SIMModem::isServiceConnected()
{
    String response = sendCommand("AT+CGATT?");
    String responseCode = response.substring(8);
    return responseCode == "1";
}

unsigned long baudRate[7] = {4800, 9600, 19200, 38400, 57600, 115200, 230400};

bool SIMModem::isModemOnline()
{
    bool connected = false;
    connected = sendCommand("AT", S_OK, 0, 500, false) == S_OK;

    if (connected)
    {
        m_console->println("modem=online;");
        return true;
    }
    else
    {
        m_console->printError("modem=offline;");
    }

    return connected;

    int idx = 0;
    while (!connected && idx < 7)
    {
        connected = sendCommand("AT", S_OK, 0, 500, false) == S_OK;

        if (!connected)
        {
            m_console->printWarning("modem=offline; // trying baud: " + String(baudRate[idx]));
            delay(500);

            m_channel->setBaudRate(baudRate[idx]);
            idx++;
        }
    }

    m_console->printWarning("modemonline=" + connected ? "true" : "false");
    return connected;
}

bool SIMModem::selectNetwork()
{
    /* Connect to AT&T network */
    if (sendCommand("AT+COPS=4,2,\"310410\"", S_OK, 0, 500, false) == S_OK)
    {
        m_console->printVerbose("modem=network;");
        return true;
    }
    else
    {
        m_lastError = "COMM018";
        return false;
    }
}

bool SIMModem::setNBIoTMode()
{
    if (sendCommand("AT+CMNB=1", S_OK, 0, 500, false) == S_OK)
    {
        m_console->printVerbose("modem=setnbiot;");
        return true;
    }

    m_lastError = "COMMS015";
    return false;
}

bool SIMModem::setLTE()
{
    if (sendCommand("AT+CNMP=38", S_OK, 0, 500, false) == S_OK)
    {
        m_console->printVerbose("modem=setlte;");
        return true;
    }

    m_lastError = "COMMS014";

    return false;
}

bool SIMModem::resetModem()
{
    m_isHttpSessionActive = false;
    m_isSecureHttpSession = false;
    m_isCellularOtaCaReady = false;
    m_secureOrigin = "";
    m_secureHost = "";
    String response = sendCommand("AT+CFUN=1,1", S_OK, 0, 15000, false);
    if (response == S_NOSIM)
    {
        bool toggle = false;
        while (true)
        {
            #ifdef LCD_DISPLAY    
            m_display->drawStr("ERROR", "NO SIM INSERTED");
            if (toggle)
                m_display->drawStr("ERROR", "NO SIM INSERTED", "!!!!!");
            else
                m_display->drawStr("ERROR", "NO SIM INSERTED");
            #endif

            m_console->printError("modemreset=error; // NO SIM INSERTED!");
            toggle = !toggle;
            delay(500);
        }
        return false;
    }
    else if (response == S_OK || response == S_RDY)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool SIMModem::setPDPContext()
{
    //sendCommand("AT+CGDCONT=?");
    m_console->printVerbose("existingvalue;");
    uint8_t retryCount = 0;
    while (retryCount < 5)
    {
        String str = "AT+CGDCONT=1,\"IP\",\"hologram\"";//,\"0.0.0.0\",0,0,0,0";
        //String str = "AT+CGDCONT=1,\"IP\",,,0,0,0,0";
        if (sendCommand(str) == S_OK)
        {
            m_console->printVerbose("modem=pdpcontext;");
            return true;
        }

        delay(1500);
    }

    m_lastError = "COMMS0019";
    m_console->printError("modem=couldnotsetpdpcontext;");
    return false;
}

bool SIMModem::setBand()
{
    int retry = 0;
    while (retry < 5)
    {
        if (sendCommand("AT+CBAND=\"ALL_MODE\"") == S_OK)
        {
            m_console->println("modem=setband;");
            return true;
        }

        delay(1500);
    }

    m_lastError = "COMMS016";
    m_console->printError("modem=couldnotsetband;");

    return false;
}

bool SIMModem::setAPN()
{
    String desiredAPN;
    if (m_apnUid != NULL && m_apnUid.length() > 0 &&
        m_apnPwd != NULL && m_apnPwd.length() > 0)
    {
        desiredAPN = "\"" + String(m_apn) + "\",\"" + m_apnUid + "\",\"" + m_apnPwd + "\"";
    }
    else
    {
        desiredAPN = "\"" + String(m_apn) + "\"";
    }

    String currentAPN = sendCommand("AT+CSTT?");
    currentAPN = currentAPN.substring(7);

    if (currentAPN != desiredAPN)
    {
        if (sendCommand("AT+CSTT=" + desiredAPN) == S_OK)
        {
            m_console->println("modem=setapn; // " + desiredAPN + ";");
            return true;
        }
        else
        {
            m_lastError = "COMMS017";
            return false;
        }
    }
    else
    {
        m_console->println("modem=apncorrect; // " + currentAPN + ";");
    }

    return true;
}

bool SIMModem::setBearer()
{
    if (sendCommand("AT+SAPBR=3,1,\"APN\",\"" + m_apn + "\"") != S_OK)
    {
        m_console->printError("Could not set SAPBR APN");
        return false;
    }

    if (m_apnUid != NULL && m_apnUid.length() > 0 &&
        m_apnPwd != NULL && m_apnPwd.length() > 0)
    {
        if (sendCommand("AT+SAPBR=3,1,\"USER\",\"" + m_apn + "\"") != S_OK)
        {
            m_console->printError("Could not set SAPBR USER");
            return false;
        }

        if (sendCommand("AT+SAPBR=3,1,\"PWD\",\"" + m_apnUid + "\"") != S_OK)
        {
            m_console->printError("Could not set SAPBR PWD");
            return false;
        }
    }

    String response = sendCommand("AT+SAPBR=1,1");
    if (response != S_OK)
    {
        m_console->printError("openbearer=failed; // Response: " + response);
        return false;
    }

    response = sendCommand("AT+SAPBR=2,1", S_OK, 0, 1500, false);
    if (response != S_OK)
    {
        m_console->printError("querybearer=failed; // Response: " + response);
        return false;
    }

    return true;
}

bool SIMModem::disconnectIP()
{
    return (sendCommand("AT+CIPSHUT", S_SHUT_OK, 0, 5000, false) != S_OK);
}

bool SIMModem::getCGREG()
{
    String creg = sendCommand("AT+CGREG?");
    bool success = creg.length() >= 10;
    if(success) {
        success = (creg.charAt(10) == '1' || creg.charAt(10) == '5');
    }

    if (!success){
        m_console->printError("AT+CREG=failed; // Response: " + creg);
    }

    return success;
}

void SIMModem::alloc(){
   if(m_rxBuffer == NULL)
        m_rxBuffer = (byte*)malloc(DOWNLOAD_BUFFER_SIZE);

    if(m_tempBuffer == NULL)
        m_tempBuffer = (byte*)malloc(TEMP_BUFFER_SIZE);
}

bool SIMModem::init()
{
    m_console->printVerbose("Initialization of SIM Modem Started.");

    if (!setBand())
    {
        m_lastError = "Could not set band.";
        m_console->printError("Could not set band.");
        return false;
    }

    if (!setLTE())
    {
        m_lastError = "Could not set LTE.";
        m_console->printError("Could not set LTE.");
        return false;
    }

    if (!setNBIoTMode())
    {
        m_lastError = "Could not set NBIoT Mode.";
        m_console->printError(m_lastError);
        return false;
    }

    m_simId = getSIMId();

    if (!setPDPContext())
    {
        m_lastError = "Could not set PDP Context.";
        m_console->printError(m_lastError);
        return false;
    }

    m_console->printVerbose("Initialization of SIM Modem Completed.");

    return true;
}

String SIMModem::getSIMId()
{
    m_simId = sendCommand("AT+CCID");
    return m_simId;
}

int SIMModem::getSignalQuality()
{
    return m_rssi;
}

int SIMModem::findRSSI()
{
    String csq = sendCommand("AT+CSQ");
    if (csq.length() > 0)
    {
        csq = csq.substring(6);
        m_rssi = csq.substring(0, csq.charAt(1) == ',' ? 1 : 2).toInt();
        return m_rssi;
    }

    return -1;
}

String SIMModem::getIPAddress()
{
    return m_ipAddress;
}

String SIMModem::parseIPAddress()
{
    String ipAddress = sendCommand("AT+CIFSR", "", 0, 500, true);
    if (ipAddress == "0.0.0.0")
    {
        return "";
    }
    else
    {
        /* yeah, this sorta sux, but it checks that the string is in four
         * segments seperated by .
         * guess it gets the job done.
         */
        int idx1 = ipAddress.indexOf('.');
        if (idx1 > -1)
        {
            int idx2 = ipAddress.indexOf('.', idx1 + 1);
            if (idx2 > -1)
            {
                int idx3 = ipAddress.indexOf('.', idx2 + 1);
                if (idx3 > -1)
                {
                    return ipAddress;
                }
            }
        }
    }

    return "";
}

bool SIMModem::connectServer(String hostName, String port)
{
    String response = sendCommand("AT+CIPSHUT", S_SHUT_OK, 0, 5000, false);

    if (response != S_OK)
    {
        m_console->printError("connectserver=fail; // SHUT OK unexpected response- " + response);
        return false;
    }

    response = sendCommand("AT+CIPCLOSE", S_CLOSE_OK, 0, 5000, false);

    // hack: if the connection is not open, we probably will get an error, that's OK.
    // todo: get CIPSTATUS working, maybe that will clean this up a little.
    if (response != S_OK && response != S_ERROR)
    {
        m_console->printError("connectserver=fail; // CLOSE OK unexpected response - " + response);
        return false;
    }

    String connect = "AT+CIPSTART=\"TCP\",\"" + hostName + "\",\"" + port + "\"";

    response = sendCommand(connect, S_CONNECT_OK, 0, 15000, false);

    if (response != S_OK)
    {
        m_console->printError("connectserver=fail; // CONNECT OK unexpected response - " + response);
        return false;
    }

    return true;
}

bool SIMModem::disconnectServer()
{
    String connect = "AT+CIPCLOSE";
    String response = sendCommand(connect, S_CLOSE_OK, 0, 5000, false);

    if (response != S_OK && response != S_CLOSE_OK)
    {
        m_console->printError("disconnectserver=fail; // Disconnect OK unexpected response - " + response);
        return false;
    }

    m_console->println("disconnectserver=success;");
    return true;
}

bool SIMModem::setBaudRate(unsigned long baudRate)
{
    String baudRateCmd = "AT+IPR=" + String(baudRate);
    return sendCommand(baudRateCmd);
}

bool SIMModem::initGPS()
{
    if (sendCommand("AT+CGNSPWR=1", "OK", 0, 1500, false) != S_OK)
    {
        m_console->printError("gps=couldnotpoweron;");
        return false;
    }
    else
    {
        m_console->println("gps=poweredon;");
    }

    if (sendCommand("AT+CGNSCOLD", "OK", 0, 1500, false) != S_OK)
    {
        m_console->printError("gps=couldnotcoldstart;");
        return false;
    }
    else
    {
        m_console->println("gps=coldstart;");
    }

    return true;
}

void SIMModem::startGPS()
{
    initGPS();
}

void SIMModem::stopGPS()
{
    if (sendCommand("AT+CGNSURC=0", S_OK, 0, 1500, false) != S_OK)
    {
        m_console->printError("Could not open Bearer");
    }
}

GPSData *SIMModem::readGPS(){
    m_channel->print("AT+CGNSINF\r");
    String echo = m_channel->readStringUntil('\r', 100);
    m_channel->waitForCRLF();

    String gpsData = m_channel->readStringUntil('\r', 100);
    m_channel->waitForLF();
    m_channel->waitForCRLF();

    m_console->println(gpsData);

    String ok = m_channel->readStringUntil('\r', 100);
    m_channel->waitForLF();

    if (ok == "OK")
    {
        m_console->printVerbose("gps=readdata;");
        m_gpsData->parse(gpsData + "\r");
        return m_gpsData;
    }
    else
    {
        m_console->printError("gps=couldnotreaddata;");
        return NULL;
    }
}

bool SIMModem::connect(String apn, String apnUid, String apnPwd)
{
    if (!init())
    {
        return false;
    }

    m_apn = apn;
    m_apnUid = apnUid;
    m_apnPwd = apnPwd;

    boolean connected = false;
    uint8_t retryCount = 0;
    while (!connected)
    {
        m_console->printVerbose("Connect to cell service.");
        while (!getCGREG())
        {
            retryCount++;
            m_console->printWarning("Not connect to cell service.");
            delay(retryCount * 250);

            if (retryCount > 20)
            {
                m_lastError = "SIM0010";
                return false;
            }
        }

        m_network = getNetwork();
        m_console->printVerbose("Connected to network: " + m_network);
        m_console->println("network=" + m_network + ";");

        int signalStrength = findRSSI();
        while (signalStrength < 1)
        {
            signalStrength = findRSSI();
        }

        m_console->printVerbose("Signal strength: " + String(signalStrength));
        m_console->println("signalstrength=" + String(signalStrength) + ";");

        setAPN();

        retryCount = 0;
        while (!isServiceConnected())
        {
            retryCount++;
            delay(retryCount * 250);
            m_console->printWarning("Service status is not up, retrying.");

            if (retryCount > 20)
            {
                m_lastError = "SIM0011";
                m_console->printError("connect=failed; // service not connected.");
                return false;
            }
        }

        m_console->println("servicestatus=up;");
        retryCount = 0;
        m_ipAddress = "";
        if (m_ipAddress == "")
        {
            bool isGPRSConnected = false;
            while (!isGPRSConnected)
            {
                retryCount++;
                delay(retryCount * 100);
                m_console->printVerbose("GPRS Not Connected - Connecting.");
            
                isGPRSConnected = connectGPRS();

                if (retryCount > 20)
                {
                    m_lastError = "SIM0012";
                    m_console->printError("Could not connect to GPRS.");
                    return false;
                }
            }

            m_console->printVerbose("GRPS Now connected.");

            m_ipAddress = parseIPAddress();
            m_console->printVerbose("Now Connected IP Address is " + m_ipAddress);
        }
        else
        {
            m_console->printVerbose("Already Connected IP Address is " + m_ipAddress);
        }

        connected = true;
    }

    return connected;
}