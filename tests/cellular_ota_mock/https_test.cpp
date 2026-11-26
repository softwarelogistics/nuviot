#include <cassert>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <string>
#include <vector>
#include "OtaTransfer.h"

#define NUVIOT_CELLULAR_OTA_TRUST_ANCHOR_PEM "TEST_CA"
#include "CellularOtaTrust.h"
typedef uint8_t byte;
#define S_OK "OK"
#define S_ERROR "ERROR"
#define DOWNLOAD_BUFFER_SIZE 16384l

struct String
{
    std::string s;
    String(const char *v = "") : s(v ? v : "") {}
    String(const std::string &v) : s(v) {}
    String(unsigned long n) : s(std::to_string(n)) {}
    String(unsigned int n) : s(std::to_string(n)) {}
    size_t length() const { return s.size(); }
    const char *c_str() const { return s.c_str(); }
    char charAt(size_t i) const { return i < s.size() ? s[i] : 0; }
    String operator+(const String &v) const { return s + v.s; }
    friend String operator+(const char *a, const String &b) { return std::string(a) + b.s; }
    bool operator==(const String &v) const { return s == v.s; }
    bool operator!=(const String &v) const { return s != v.s; }
    void trim()
    {
        const auto first = s.find_first_not_of(" \r\n\t");
        if (first == std::string::npos) { s.clear(); return; }
        const auto last = s.find_last_not_of(" \r\n\t");
        s = s.substr(first, last - first + 1);
    }
    bool startsWith(const char *p) const { return s.find(p) == 0; }
    int indexOf(char c, int start = 0) const
    {
        const auto at = s.find(c, static_cast<size_t>(start));
        return at == std::string::npos ? -1 : static_cast<int>(at);
    }
    String substring(int start, int end = -1) const
    {
        if (start < 0) start = 0;
        if (end < 0) return s.substr(static_cast<size_t>(start));
        return s.substr(static_cast<size_t>(start), static_cast<size_t>(end - start));
    }
};

struct Console
{
    std::vector<std::string> messages;
    void println(String s) { messages.push_back(s.s); }
    void printWarning(String s) { messages.push_back(s.s); }
    void printError(String s) { messages.push_back(s.s); }
};

struct Hal
{
    int feeds = 0;
    void feedHWWatchdog() { ++feeds; }
};

unsigned long tick = 0;
unsigned long millis() { return tick; }
void delay(unsigned long ms) { tick += ms; }

struct Channel
{
    std::deque<String> lines;
    std::vector<std::string> commands;
    std::string responseBody;
    std::string certificateBytes;
    size_t bodyOffset = 0;
    int responseStatus = 200;
    bool secureConnectOk = true;
    bool clockValid = true;
    int ntpCode = 1;

    String command(String cmd)
    {
        commands.push_back(cmd.s);
        if (cmd.startsWith("AT+CCLK?"))
            return clockValid ? String("+CCLK: \"26/11/26,10:00:00+00\"") :
                String("+CCLK: \"00/01/01,00:00:00+00\"");
        if (cmd.startsWith("AT+CGMM")) return "SIMCOM_SIM7000G";
        if (cmd.startsWith("AT+CGMR")) return "R1351";
        if (cmd == "AT+CFSWFILE=3,\"nuviot-dfu-root-v1.cer\",0,7,1000") return "DOWNLOAD";
        if (cmd == "AT+SHCONN") return secureConnectOk ? String("OK") : String("ERROR");
        if (cmd == "AT+SHDISC") return "OK";
        if (cmd == "AT+HTTPTERM") return "OK";
        if (cmd == "AT+CFSDFILE=3,\"nuviot-dfu-root-v1.cer\"") return "ERROR";
        return "OK";
    }

    void transmit(String cmd)
    {
        commands.push_back(cmd.s);
        if (cmd == "AT+CNTP\r\n")
        {
            lines.push_back("OK");
            lines.push_back(String("+CNTP: ") + String(static_cast<unsigned int>(ntpCode)));
            return;
        }
        if (cmd.startsWith("AT+SHREQ=\""))
        {
            lines.push_back("OK");
            lines.push_back(String("+SHREQ: \"GET\",") + String(static_cast<unsigned int>(responseStatus)) +
                "," + String(static_cast<unsigned long>(responseBody.size())));
            bodyOffset = 0;
            return;
        }
        if (cmd.startsWith("AT+SHREAD="))
        {
            const size_t equals = cmd.s.find('=');
            const size_t comma = cmd.s.find(',', equals + 1);
            const size_t offset = static_cast<size_t>(std::stoul(cmd.s.substr(equals + 1, comma - equals - 1)));
            const size_t length = static_cast<size_t>(std::stoul(cmd.s.substr(comma + 1)));
            assert(offset + length <= responseBody.size());
            bodyOffset = offset;
            lines.push_back("OK");
            lines.push_back(String("+SHREAD: ") + String(static_cast<unsigned long>(length)));
            return;
        }
        if (cmd.startsWith("AT+HTTPACTION=0"))
        {
            lines.push_back("+HTTPACTION: 0,200,0");
            lines.push_back("OK");
        }
    }

    void transmit(byte *data, size_t length)
    {
        certificateBytes.append(reinterpret_cast<const char *>(data), length);
        lines.push_back("OK");
    }

    String readStringUntil(char, int)
    {
        if (lines.empty()) return "";
        String line = lines.front();
        lines.pop_front();
        return line;
    }

    size_t readBytes(byte *buffer, size_t length)
    {
        if (bodyOffset + length > responseBody.size()) return static_cast<size_t>(-1);
        memcpy(buffer, responseBody.data() + bodyOffset, length);
        bodyOffset += length;
        return length;
    }

    void clearBuffers() { lines.clear(); bodyOffset = 0; }
};

struct SIMModem
{
    Channel *m_channel;
    Hal *m_hal;
    Console *m_console;
    bool m_isHttpSessionActive = false;
    bool m_isSecureHttpSession = false;
    bool m_isCellularOtaCaReady = false;
    String m_secureOrigin;
    String m_secureHost;
    uint8_t rx[DOWNLOAD_BUFFER_SIZE];
    uint8_t temp[2048];

    SIMModem(Channel *channel, Hal *hal, Console *console)
        : m_channel(channel), m_hal(hal), m_console(console) {}

    String sendCommand(String command)
    {
        const String response = m_channel->command(command);
        return response == "DOWNLOAD" ? String("DOWNLOAD") : response;
    }

    String sendCommand(String command, String expectedReply, unsigned long, long, bool returnAny)
    {
        const String response = m_channel->command(command);
        if (returnAny) return response;
        if (response == expectedReply) return S_OK;
        return response;
    }

    bool parseSecureUrl(String, String &, String &, String &);
    bool installCellularOtaTrustAnchor();
    bool isCellularModemClockValid();
    bool synchronizeCellularModemClock();
    bool configureSecureHttpContext(String);
    int64_t configureSecureForDownload(String, size_t, bool, bool);
    int64_t readSecureHttpContent(uint32_t, unsigned char *);
    int64_t configureForDownload(String, size_t, bool = true, bool = false);
    bool closeHttpContext(String);
};

#include "https_modem_excerpt.cpp"

bool hasCommand(const Channel &channel, const std::string &prefix)
{
    for (const auto &command : channel.commands)
        if (command.find(prefix) == 0) return true;
    return false;
}

void secureBinaryTransfer()
{
    Channel channel;
    Hal hal;
    Console console;
    SIMModem modem(&channel, &hal, &console);

    channel.responseBody = "12345";
    const String base = "https://dfu-dev.nuviot.com/api/firmware/download/test-request";
    const int64_t size = modem.configureForDownload(base + "/size", 32);
    assert(size == 5);

    uint8_t sizeBody[8] = {};
    assert(modem.readSecureHttpContent(5, sizeBody) == 5);
    assert(std::string(reinterpret_cast<char *>(sizeBody), 5) == "12345");

    std::string binary(4097, '\0');
    for (size_t i = 0; i < binary.size(); ++i)
        binary[i] = static_cast<char>((i * 37) & 0xff);
    channel.responseBody = binary;

    const int64_t chunk = modem.configureForDownload(base + "?start=0&length=4097", binary.size(), true, true);
    assert(chunk == static_cast<int64_t>(binary.size()));

    std::vector<uint8_t> received(binary.size());
    assert(modem.readSecureHttpContent(static_cast<uint32_t>(binary.size()), received.data()) ==
        static_cast<int64_t>(binary.size()));
    assert(memcmp(received.data(), binary.data(), binary.size()) == 0);

    assert(modem.m_isHttpSessionActive && modem.m_isSecureHttpSession);
    assert(modem.closeHttpContext("test"));
    assert(!modem.m_isHttpSessionActive && !modem.m_isSecureHttpSession);
    assert(channel.certificateBytes == "TEST_CA");
    assert(hasCommand(channel, "AT+CSSLCFG=\"sslversion\",1,3"));
    assert(hasCommand(channel, "AT+CSSLCFG=\"ignorertctime\",1,0"));
    assert(hasCommand(channel, "AT+CSSLCFG=\"sni\",1,\"dfu-dev.nuviot.com\""));
    assert(hasCommand(channel, "AT+SHSSL=1,\"nuviot-dfu-root-v1.cer\""));
    assert(hasCommand(channel, "AT+SHCONF=\"URL\",\"https://dfu-dev.nuviot.com\""));
    assert(hasCommand(channel, "AT+SHREAD=0,"));
    assert(hasCommand(channel, "AT+SHREAD=2048,"));
    assert(hasCommand(channel, "AT+SHREAD=4096,"));
    assert(hasCommand(channel, "AT+CNTP\\r\\n"));
    assert(hasCommand(channel, "AT+CCLK?"));
    assert(hasCommand(channel, "AT+SHREQ=\"https://dfu-dev.nuviot.com/api/firmware/download/test-request/size\",1"));
    for (const auto &message : console.messages)
        assert(message.find("test-request") == std::string::npos);
    assert(!hasCommand(channel, "AT+HTTPINIT"));
    assert(!hasCommand(channel, "AT+HTTPACTION"));
    assert(hal.feeds > 0);
}

void rejectsUntrustedCertificate()
{
    Channel channel;
    Hal hal;
    Console console;
    SIMModem modem(&channel, &hal, &console);
    channel.secureConnectOk = false;
    assert(modem.configureForDownload("https://dfu-dev.nuviot.com/api/firmware/download/id/size", 32) == -1);
    assert(!modem.m_isHttpSessionActive);
    assert(hasCommand(channel, "AT+SHCONN"));
    assert(!hasCommand(channel, "AT+HTTPINIT"));
    assert(!hasCommand(channel, "AT+HTTPACTION"));
}

void rejectsHostnameMismatch()
{
    Channel channel;
    Hal hal;
    Console console;
    SIMModem modem(&channel, &hal, &console);
    channel.secureConnectOk = false;
    assert(modem.configureForDownload("https://dfu-dev.nuviot.com/api/firmware/download/id/size", 32) == -1);
    assert(!modem.m_isHttpSessionActive);
    assert(hasCommand(channel, "AT+CSSLCFG=\"sni\",1,\"dfu-dev.nuviot.com\""));
    assert(hasCommand(channel, "AT+SHCONN"));
    assert(!hasCommand(channel, "AT+HTTPINIT"));
}

void rejectsInvalidClock()
{
    Channel channel;
    Hal hal;
    Console console;
    SIMModem modem(&channel, &hal, &console);
    channel.clockValid = false;
    channel.ntpCode = 62;
    assert(modem.configureForDownload("https://dfu-dev.nuviot.com/api/firmware/download/id/size", 32) == -1);
    assert(hasCommand(channel, "AT+CNTP\r\n"));
    assert(!hasCommand(channel, "AT+SHCONN"));
    assert(!hasCommand(channel, "AT+HTTPINIT"));
}

void requiresFreshNtpEvenWithPlausibleRtc()
{
    Channel channel;
    Hal hal;
    Console console;
    SIMModem modem(&channel, &hal, &console);
    channel.clockValid = true;
    channel.ntpCode = 62;
    assert(modem.configureForDownload("https://dfu-dev.nuviot.com/api/firmware/download/id/size", 32) == -1);
    assert(hasCommand(channel, "AT+CNTP\\r\\n"));
    assert(!hasCommand(channel, "AT+SHCONN"));
    assert(!hasCommand(channel, "AT+HTTPINIT"));
}

void rejectsMalformedUrl()
{
    Channel channel;
    Hal hal;
    Console console;
    SIMModem modem(&channel, &hal, &console);
    String origin, host, path;
    assert(!modem.parseSecureUrl("http://dfu-dev.nuviot.com/path", origin, host, path));
    assert(!modem.parseSecureUrl("https://user@dfu-dev.nuviot.com/path", origin, host, path));
    assert(!modem.parseSecureUrl("https://dfu-dev.nuviot.com/path#fragment", origin, host, path));
    assert(!modem.parseSecureUrl("https://dfu-dev.nuviot.com:0/path", origin, host, path));
    assert(modem.parseSecureUrl("https://dfu-dev.nuviot.com/api/path?q=1", origin, host, path));
    assert(origin == "https://dfu-dev.nuviot.com");
    assert(host == "dfu-dev.nuviot.com");
    assert(path == "/api/path?q=1");
}

void rejectsHttpRedirect()
{
    Channel channel;
    Hal hal;
    Console console;
    SIMModem modem(&channel, &hal, &console);
    channel.responseStatus = 302;
    assert(modem.configureForDownload("https://dfu-dev.nuviot.com/api/firmware/download/id/size", 32) == -1);
    assert(modem.m_isHttpSessionActive);
    assert(modem.closeHttpContext("redirect"));
}

int main()
{
    secureBinaryTransfer();
    rejectsUntrustedCertificate();
    rejectsHostnameMismatch();
    rejectsInvalidClock();
    requiresFreshNtpEvenWithPlausibleRtc();
    rejectsMalformedUrl();
    rejectsHttpRedirect();
    std::cout << "7 cellular HTTPS adapter scenarios passed\n";
}
