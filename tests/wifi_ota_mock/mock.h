#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <cassert>
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
 String operator+(const String& x) const {String t; t.s=s+x.s; return t;}
 bool operator==(const char *x) const {return s==(x?x:"");}
};
class Stream {public: virtual ~Stream(){}; virtual int available()=0; virtual int read()=0; virtual int peek()=0; virtual void flush(){}; virtual size_t write(uint8_t)=0; virtual size_t write(const uint8_t*,size_t)=0;};
struct Response {std::string url; int status; int declared; std::string body; bool begin=true; int result=0; Response(std::string u,int st,int d,std::string b,bool ok=true,int res=0):url(u),status(st),declared(d),body(b),begin(ok),result(res){} };
extern std::deque<Response> replies;
extern int opens, closes, getCount;
class HTTPClient {
 Response r{"",0,0,""}; bool active=false;
public:
 void setReuse(bool x){assert(!x);} void setConnectTimeout(int x){assert(x==5000);} void setTimeout(unsigned x){assert(x==5000);}
 bool begin(String u){assert(!active);assert(!replies.empty()); r=replies.front(); replies.pop_front(); assert(r.url==u.s); active=true; ++opens; return r.begin;}
 int GET(){assert(active); ++getCount; return r.status;} int getSize(){assert(active);return r.declared;}
 int writeToStream(Stream *s){assert(active);size_t n=s->write(reinterpret_cast<const uint8_t*>(r.body.data()),r.body.size()); if(n!=r.body.size()) return -10;return r.result?r.result:static_cast<int>(n);}
 void end(){assert(active); active=false;++closes;}
};
struct Console{std::vector<std::string> logs;void println(String s){logs.push_back(s.s);}void printError(String s){logs.push_back(s.s);}void enableBTOut(bool b){assert(!b);}};
struct Hal{int restarts=0,feeds=0,watchdogs=0;void enableHWWatchdog(long x){assert(x==120);++watchdogs;}void feedHWWatchdog(){++feeds;}void restart(){++restarts;}};
struct NuvIoTState {int OTAState=0;};
struct Display {void clearBuffer(){};void drawStr(const char*,const char* = nullptr){};void sendBuffer(){};};
struct SIMModem {int calls=0; bool beginDownload(String){++calls; return true;}};
struct esp_partition_t{size_t size;}; extern esp_partition_t partition;extern bool hasPartition;
inline const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t*){return hasPartition?&partition:nullptr;}
inline void delay(unsigned long){}
struct UpdateMock{int begins=0,ends=0,aborts=0,writes=0;size_t total=0,done=0;bool beginOk=true,writeOk=true,endOk=true,finishOk=true;bool begin(size_t n,int){++begins;total=n;return beginOk;}size_t write(uint8_t*,size_t n){++writes;if(!writeOk)return n-1;done+=n;return n;}bool isFinished(){return finishOk&&done==total;}bool end(){++ends;return endOk;}void abort(){++aborts;}};
extern UpdateMock Update;
