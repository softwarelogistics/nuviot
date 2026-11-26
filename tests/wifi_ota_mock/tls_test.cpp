#include "mock.h"
#include "OtaServices.h"
#include <iostream>

std::deque<Response> replies;
int opens=0, closes=0, getCount=0, plainBegins=0, secureBegins=0;
UpdateMock Update;
esp_partition_t partition{16384};
bool hasPartition=true;
mbedtls_x509_crt peerCertificate{{2025,1,1,0,0,0},{2027,1,1,0,0,0},nullptr};
mbedtls_x509_crt intermediateCertificate{{2024,1,1,0,0,0},{2027,1,1,0,0,0},nullptr};
bool tlsTrustOk=true, tlsHostnameOk=true, ntpSyncOk=true;
int ntpCalls=0, secureClientConstructions=0, handshakeTimeoutSeconds=0, redirectsDisabledCalls=0;
time_t fakeNow=static_cast<time_t>(1795651200);
unsigned long fakeMillis=0;
std::string lastTlsHost;
uint16_t lastTlsPort=0;
bool lastTlsUsedBundle=false;

const std::string httpsUrl="https://ota.example/api/firmware/download/test-id";
std::string activeUrl=httpsUrl;

void reset()
{
    replies.clear();
    opens=closes=getCount=plainBegins=secureBegins=0;
    Update=UpdateMock{};
    hasPartition=true;
    partition.size=16384;
    peerCertificate.valid_from={2025,1,1,0,0,0};
    peerCertificate.valid_to={2027,1,1,0,0,0};
    peerCertificate.next=nullptr;
    intermediateCertificate.valid_from={2024,1,1,0,0,0};
    intermediateCertificate.valid_to={2027,1,1,0,0,0};
    intermediateCertificate.next=nullptr;
    tlsTrustOk=tlsHostnameOk=ntpSyncOk=true;
    ntpCalls=secureClientConstructions=handshakeTimeoutSeconds=redirectsDisabledCalls=0;
    fakeNow=static_cast<time_t>(1795651200);
    fakeMillis=0;
    lastTlsHost.clear();
    lastTlsPort=0;
    lastTlsUsedBundle=false;
    activeUrl=httpsUrl;
}

void size(std::string body,int declared=-2,int status=200,bool tls=true)
{
    replies.emplace_back(activeUrl+"/size",status,declared==-2?static_cast<int>(body.size()):declared,body,true,0,tls);
}

void block(int offset,int length,int status=200,int actual=-1,int declared=-1,bool tls=true)
{
    replies.emplace_back(activeUrl+"?start="+std::to_string(offset)+"&length="+std::to_string(length),status,
        declared==-1?length:declared,std::string(actual==-1?length:actual,'x'),true,0,tls);
}

void terminal(bool ok,int status=200,bool tls=true)
{
    replies.emplace_back(activeUrl+(ok?"/success":"/failed"),status,0,"",true,0,tls);
}

void runHttps(bool expectedSuccess,int begins,int writes,int ends,int aborts)
{
    Console c;
    Hal h;
    NuvIoTState s;
#ifdef LCD_DISPLAY
    Display d;
    OtaServices ota(&d,&c,&s,&h);
#else
    OtaServices ota(&c,&s,&h);
#endif
    assert(ota.downloadOverWiFi(String(httpsUrl.c_str()))==expectedSuccess);
    assert(replies.empty());
    assert(opens==closes);
    assert(h.restarts==1&&h.feeds>0&&h.watchdogs==1);
    assert(Update.begins==begins&&Update.writes==writes&&Update.ends==ends&&Update.aborts==aborts);
    assert(redirectsDisabledCalls==1);
    assert(secureClientConstructions==1);
    assert(secureBegins==opens&&plainBegins==0);
    assert(lastTlsUsedBundle);
    assert(lastTlsHost=="ota.example"&&lastTlsPort==443);
    assert(handshakeTimeoutSeconds==10);
    for(auto &log:c.logs) assert(log.find("test-id")==std::string::npos);
}

int main()
{
    int count=0;

    reset();size("4096");block(0,4096);terminal(true);runHttps(true,1,1,1,0);++count;

    reset();fakeNow=0;ntpSyncOk=true;size("4096");block(0,4096);terminal(true);runHttps(true,1,1,1,0);assert(ntpCalls==1);++count;

    reset();tlsTrustOk=false;size("4096");terminal(false);runHttps(false,0,0,0,0);++count;
    reset();tlsHostnameOk=false;size("4096");terminal(false);runHttps(false,0,0,0,0);++count;
    reset();peerCertificate.valid_to={2026,10,1,0,0,0};size("4096");terminal(false);runHttps(false,0,0,0,0);++count;
    reset();peerCertificate.valid_from={2027,1,1,0,0,0};size("4096");terminal(false);runHttps(false,0,0,0,0);++count;
    reset();intermediateCertificate.valid_to={2026,10,1,0,0,0};peerCertificate.next=&intermediateCertificate;size("4096");terminal(false);runHttps(false,0,0,0,0);++count;

    reset();size("8192");block(0,4096,200,-1,-1,false);block(0,4096);block(4096,4096);terminal(true);runHttps(true,1,2,1,0);++count;
    reset();size("4096",-2,302);terminal(false);runHttps(false,0,0,0,0);++count;
    reset();size("4096");for(int i=0;i<5;++i)block(0,4096,200,-1,-1,false);terminal(false);runHttps(false,1,0,0,1);++count;
    reset();size("4096");block(0,4096);terminal(true,200,false);runHttps(true,1,1,1,0);++count;

    reset();fakeNow=0;ntpSyncOk=false;
    {
        Console c;Hal h;NuvIoTState s;
#ifdef LCD_DISPLAY
        Display d;OtaServices ota(&d,&c,&s,&h);
#else
        OtaServices ota(&c,&s,&h);
#endif
        assert(!ota.downloadOverWiFi(String(httpsUrl.c_str())));
        assert(replies.empty()&&opens==0&&closes==0&&plainBegins==0&&secureBegins==0);
        assert(Update.begins==0&&h.restarts==1&&h.feeds>0&&h.watchdogs==1&&ntpCalls==1);
        assert(redirectsDisabledCalls==1&&secureClientConstructions==1);
    }
    ++count;

    reset();
    {
        Console c;Hal h;NuvIoTState s;
#ifdef LCD_DISPLAY
        Display d;OtaServices ota(&d,&c,&s,&h);
#else
        OtaServices ota(&c,&s,&h);
#endif
        assert(!ota.downloadOverWiFi(String("ftp://ota.example/firmware")));
        assert(opens==0&&h.restarts==0&&h.watchdogs==0&&secureClientConstructions==0);
    }
    ++count;

    std::cout<<count<<" HTTPS adapter scenarios passed\n";
}
