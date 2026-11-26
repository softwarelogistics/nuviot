#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <type_traits>
#define F(x) x
#define U_FLASH 0

class String {
public:
 std::string s;
 String() = default;
 String(const char *x):s(x?x:""){}
 template<class T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0> String(T x):s(std::to_string(x)){}
 size_t length() const {return s.size();}
 bool startsWith(const char *x) const {return s.compare(0, std::strlen(x), x) == 0;}
 String operator+(const String& x) const {String t; t.s=s+x.s; return t;}
 bool operator==(const char *x) const {return s==(x?x:"");}
};

struct mbedtls_x509_time { int year, mon, day, hour, min, sec; };
struct mbedtls_x509_crt { mbedtls_x509_time valid_from, valid_to; mbedtls_x509_crt *next; };
extern mbedtls_x509_crt peerCertificate;
extern bool tlsTrustOk, tlsHostnameOk;
extern bool ntpSyncOk;
extern int ntpCalls;
extern time_t fakeNow;
extern unsigned long fakeMillis;
extern std::string lastTlsHost;
extern uint16_t lastTlsPort;
extern bool lastTlsUsedBundle;
extern int secureClientConstructions;
extern int handshakeTimeoutSeconds;
extern int redirectsDisabledCalls;

enum { HTTPC_DISABLE_FOLLOW_REDIRECTS = 0 };

class Stream {
public:
 virtual ~Stream(){}
 virtual int available()=0;
 virtual int read()=0;
 virtual int peek()=0;
 virtual void flush(){}
 virtual size_t write(uint8_t)=0;
 virtual size_t write(const uint8_t*,size_t)=0;
};

struct IPAddress {};

class WiFiClient {
protected:
 bool mockConnectResult = true;
public:
 virtual ~WiFiClient(){}
 virtual int connect(const char *, uint16_t) { return mockConnectResult ? 1 : 0; }
 virtual int connect(IPAddress, uint16_t) { return 0; }
 virtual int connect(const char *host, uint16_t port, int32_t) { return connect(host, port); }
 virtual void stop() {}
 virtual void setMockConnectResult(bool value) { mockConnectResult = value; }
 virtual bool isSecure() const { return false; }
};

class WiFiClientSecure : public WiFiClient {
protected:
 bool _use_ca_bundle = false;
public:
 WiFiClientSecure() { ++secureClientConstructions; }
 void setHandshakeTimeout(unsigned long seconds) { handshakeTimeoutSeconds = static_cast<int>(seconds); }
 int setTimeout(uint32_t) { return 1; }
 int connect(const char *host, uint16_t port) override {
   lastTlsHost = host ? host : "";
   lastTlsPort = port;
   lastTlsUsedBundle = _use_ca_bundle;
   return (_use_ca_bundle && mockConnectResult && tlsTrustOk && tlsHostnameOk) ? 1 : 0;
 }
 int connect(const char *host, uint16_t port, int32_t timeout) override {
   return WiFiClient::connect(host, port, timeout);
 }
 void stop() override {}
 bool isSecure() const override { return true; }
 const mbedtls_x509_crt *getPeerCertificate() const { return &peerCertificate; }
};

struct Response {
 std::string url;
 int status;
 int declared;
 std::string body;
 bool begin = true;
 int result = 0;
 bool tlsConnectOk = true;
 Response(std::string u, int st, int d, std::string b, bool ok=true, int res=0, bool tls=true)
   : url(u), status(st), declared(d), body(b), begin(ok), result(res), tlsConnectOk(tls) {}
};
extern std::deque<Response> replies;
extern int opens, closes, getCount, plainBegins, secureBegins;
extern bool hasPartition;

class HTTPClient {
 Response r{"",0,0,""};
 bool active=false;
 WiFiClient *externalClient=nullptr;
public:
 void setReuse(bool x){assert(!x);}
 void setConnectTimeout(int x){assert(x==5000);}
 void setTimeout(unsigned x){assert(x==5000);}
 void setFollowRedirects(int value){assert(value==HTTPC_DISABLE_FOLLOW_REDIRECTS);++redirectsDisabledCalls;}
 bool begin(String u){
   assert(!active);
   assert(u.startsWith("http://"));
   if (replies.empty()) return false;
   r=replies.front(); replies.pop_front(); assert(r.url==u.s);
   externalClient=nullptr; active=true; ++opens; ++plainBegins; return r.begin;
 }
 bool begin(WiFiClient &client, String u){
   assert(!active);
   assert(u.startsWith("https://"));
   if (replies.empty()) return false;
   r=replies.front(); replies.pop_front(); assert(r.url==u.s);
   externalClient=&client; externalClient->setMockConnectResult(r.tlsConnectOk);
   active=true; ++opens; ++secureBegins; return r.begin;
 }
 int GET(){
   assert(active); ++getCount;
   if (externalClient) {
     const size_t scheme = r.url.find("://");
     assert(scheme != std::string::npos);
     const size_t hostStart = scheme + 3;
     const size_t hostEnd = r.url.find_first_of("/:?", hostStart);
     std::string authority = r.url.substr(hostStart, hostEnd == std::string::npos ? std::string::npos : hostEnd - hostStart);
     std::string host = authority;
     uint16_t port = 443;
     const size_t colon = authority.rfind(':');
     if (colon != std::string::npos) {
       host = authority.substr(0, colon);
       port = static_cast<uint16_t>(std::atoi(authority.substr(colon + 1).c_str()));
     }
     if (!externalClient->connect(host.c_str(), port, 5000)) return -1;
   }
   return r.status;
 }
 int getSize(){assert(active);return r.declared;}
 int writeToStream(Stream *s){
   assert(active);
   size_t n=s->write(reinterpret_cast<const uint8_t*>(r.body.data()),r.body.size());
   if(n!=r.body.size()) return -10;
   return r.result?r.result:static_cast<int>(n);
 }
 void end(){
   if (!active) return;
   if (externalClient) externalClient->stop();
   externalClient=nullptr; active=false; ++closes;
 }
};

struct Console {
 std::vector<std::string> logs;
 void println(String s){logs.push_back(s.s);}
 void printError(String s){logs.push_back(s.s);}
 void enableBTOut(bool b){assert(!b);}
};
struct Hal {
 int restarts=0,feeds=0,watchdogs=0;
 void enableHWWatchdog(long x){assert(x==120);++watchdogs;}
 void feedHWWatchdog(){++feeds;}
 void restart(){++restarts;}
};
struct NuvIoTState {int OTAState=0;};
struct Display {void clearBuffer(){};void drawStr(const char*,const char* = nullptr){};void sendBuffer(){};};
struct SIMModem {int calls=0; bool beginDownload(String){++calls; return true;}};
struct esp_partition_t{size_t size;};
extern esp_partition_t partition;
inline const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t*){return hasPartition?&partition:nullptr;}
inline unsigned long millis(){return fakeMillis;}
inline void delay(unsigned long value){fakeMillis += value;}
extern "C" void configTime(long, int, const char*, const char*, const char*);

struct UpdateMock {
 int begins=0,ends=0,aborts=0,writes=0;
 size_t total=0,done=0;
 bool beginOk=true,writeOk=true,endOk=true,finishOk=true;
 bool begin(size_t n,int){++begins;total=n;return beginOk;}
 size_t write(uint8_t*,size_t n){++writes;if(!writeOk)return n-1;done+=n;return n;}
 bool isFinished(){return finishOk&&done==total;}
 bool end(){++ends;return endOk;}
 void abort(){++aborts;}
};
extern UpdateMock Update;
