#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <string>
#include <vector>
#include "OtaTransfer.h"
#include "CellularOtaTrust.h"
typedef uint8_t byte;
#define S_OK "OK"
#define S_ERROR "ERROR"
#define U_FLASH 0
#define DOWNLOAD_BUFFER_SIZE 16384l
struct String {
    std::string s;
    String(const char *v=""):s(v){} String(std::string v):s(v){}
    String(unsigned long n):s(std::to_string(n)){} String(unsigned int n):s(std::to_string(n)){}
    size_t length()const{return s.size();} const char *c_str()const{return s.c_str();}
    String operator+(const String &v)const{return s+v.s;} friend String operator+(const char *a,const String &b){return std::string(a)+b.s;}
    bool operator==(const String &v)const{return s==v.s;} bool operator!=(const String &v)const{return s!=v.s;}
    void trim(){auto a=s.find_first_not_of(" \r\n\t");s=a==std::string::npos?"":s.substr(a,s.find_last_not_of(" \r\n\t")-a+1);}
    bool startsWith(const char *p)const{return s.find(p)==0;}
    char charAt(size_t i)const{return i<s.size()?s[i]:0;}
    int indexOf(char c,int a=0)const{auto i=s.find(c,a);return i==std::string::npos?-1:static_cast<int>(i);}
    String substring(int a,int b=-1)const{return s.substr(a,b<0?std::string::npos:static_cast<size_t>(b-a));}
};
struct Console{void printWarning(String){} void printError(String){} void println(String){}};
struct Hal{int feeds=0,reboots=0;void feedHWWatchdog(){++feeds;}void enableHWWatchdog(long n){assert(n==120);}void restart(int n){assert(n==2000);++reboots;}};
struct Display{void drawStr(const char*,const char*){}};
unsigned long tick=0; unsigned long millis(){return tick;} void delay(unsigned long n){tick+=n;}
struct esp_partition_t{size_t size;};esp_partition_t partition{65536};bool hasPartition=true;
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t*){return hasPartition?&partition:nullptr;}
struct Updater{bool beginOk=true,writeOk=true,finishOk=true,endOk=true;int begins=0,writes=0,ends=0,aborts=0;size_t total=0,done=0;
 bool begin(size_t n,int){++begins;total=n;return beginOk;} size_t write(uint8_t*,size_t n){++writes;if(!writeOk)return n-1;done+=n;return n;}bool isFinished(){return finishOk&&done==total;}bool end(){++ends;return endOk;}void abort(){++aborts;}} Update;
struct Reply{std::string path,body,action,header,trailer;bool early,ack;Reply(std::string p,std::string b,std::string a="",std::string h="",std::string t="OK",bool e=false,bool k=true):path(p),body(b),action(a),header(h),trailer(t),early(e),ack(k){}};
const std::string url="http://firmware.nuviot.com:14236/api/firmware/download/mock";
struct Channel{
 std::deque<Reply> plan;std::deque<String> lines;Reply current{"",""};std::string request;size_t offset=0;int reads=0,actions=0,terms=0;bool termOk=true;
 void clearBuffers(){lines.clear();offset=0;}
 String command(String cmd){if(cmd.startsWith("AT+HTTPPARA=\"URL\",\"")){request=cmd.s.substr(19,cmd.s.size()-20);}if(cmd=="AT+HTTPTERM"){++terms;return termOk?"OK":"ERROR";}return "OK";}
 void transmit(String cmd){if(cmd=="AT+HTTPACTION=0\r\n"){assert(!plan.empty());current=plan.front();plan.pop_front();assert(request==url+current.path);++actions;offset=0;String action=current.action.empty()?String("+HTTPACTION: 0,200,"+std::to_string(current.body.size())):String(current.action);if(current.early)lines.push_back(action);if(current.ack)lines.push_back("OK");if(!current.early)lines.push_back(action);}else{assert(cmd=="AT+HTTPREAD\r\n");++reads;lines.push_back(current.header.empty()?String("+HTTPREAD: "+std::to_string(current.body.size())):String(current.header));}}
 void transmit(byte*,size_t){}
 String readStringUntil(char,int){if(lines.empty())return "";String r=lines.front();lines.pop_front();return r;}
 size_t readBytes(uint8_t *b,size_t n){assert(n<=2048);if(n>current.body.size()-offset)return size_t(-1);memcpy(b,current.body.data()+offset,n);offset+=n;if(offset==current.body.size())lines.push_back(current.trailer);return n;}
};
struct SIMModem{Channel *m_channel;Hal *m_hal;Console *m_console;Display *m_display;bool m_isHttpSessionActive=false,m_isSecureHttpSession=false,m_isCellularOtaCaReady=false;String m_secureOrigin,m_secureHost;uint8_t rx[DOWNLOAD_BUFFER_SIZE],temp[2048];uint8_t *m_rxBuffer=rx,*m_tempBuffer=temp;bool bearer=true;
 SIMModem(Channel *c,Hal *h,Console *o,Display *d):m_channel(c),m_hal(h),m_console(o),m_display(d){}
 void alloc(){}void DisconnectMQTT(){}void disconnectServer(){}bool isServiceConnected(){return bearer;}bool setBearer(){return bearer;}String sendCommand(String c){return m_channel->command(c);}String getDeviceModel(){return "SIM7000G";}String getDeviceRevision(){return "R1351";}
 int64_t configureForDownload(String,size_t,bool=true,bool=false);bool closeHttpContext(String);int64_t downloadContent(uint32_t,unsigned char*);bool httpGetNoContent(String);bool beginDownload(String);
};
#include "modem_excerpt.cpp"
std::string block(int offset,int size){return "?start="+std::to_string(offset)+"&length="+std::to_string(size);}
void run(std::deque<Reply> replies,bool ok,int begins,int writes,int ends,int aborts,int reads,bool writeOk=true,bool endOk=true,bool termOk=true,bool bearer=true){Update=Updater{};Update.writeOk=writeOk;Update.endOk=endOk;Channel c;c.plan=replies;c.termOk=termOk;Hal h;Console o;Display d;SIMModem m(&c,&h,&o,&d);m.bearer=bearer;assert(m.beginDownload(url)==ok);assert(c.plan.empty());assert(c.reads==reads);assert(Update.begins==begins&&Update.writes==writes&&Update.ends==ends&&Update.aborts==aborts);assert(h.reboots==1&&h.feeds>0);assert(!m.m_isHttpSessionActive||!termOk);}
int main(){int n=0;std::string full(16384,'x');
 run({{"/size","32768"},{block(0,16384),full},{block(16384,16384),full},{"/success",""}},true,1,2,1,0,3);++n;
 run({{"/size","16385","","","OK",true},{block(0,16384),full},{block(16384,1),"x"},{"/success",""}},true,1,2,1,0,3);++n;
 for(auto size:{"0","-1","2147483648","65537","abc"}){run({{"/size",size},{"/failed?err=invalid_firmware_size",""}},false,0,0,0,0,1);++n;}
 run({{"/size",std::string(33,'1')},{"/failed?err=invalid_firmware_size",""}},false,0,0,0,0,0);++n;
 run({{"/size","4","+HTTPACTION: 0,404,1"},{"/failed?err=invalid_firmware_size",""}},false,0,0,0,0,0);++n;
 run({{"/size","4","","","OK",false,false},{"/failed?err=invalid_firmware_size",""}},false,0,0,0,0,0);++n;
 run({{"/size","4"},{block(0,4),"abc"},{block(0,4),"abcde"},{block(0,4),"abcd","","+HTTPREAD: 3"},{block(0,4),"abc","+HTTPACTION: 0,200,4","+HTTPREAD: 4"},{block(0,4),"abcd"},{"/success",""}},true,1,1,1,0,4);++n;
 run({{"/size","4"},{block(0,4),"abcd","","","ERROR"},{block(0,4),"abcd"},{"/success",""}},true,1,1,1,0,3);++n;
 {std::deque<Reply> q{{"/size","4"}};for(int i=0;i<5;i++)q.emplace_back(block(0,4),"", "+HTTPACTION: 0,500,0");q.emplace_back("/failed?err=chunk_download_failed","");run(q,false,1,0,0,1,1);++n;}
 run({{"/size","4"},{block(0,4),"abcd"},{"/failed?err=flash_write_failed",""}},false,1,1,0,1,2,false);++n;
 run({{"/size","4"},{block(0,4),"abcd"},{"/failed?err=flash_finalize_failed",""}},false,1,1,1,1,2,true,false);++n;
 run({{"/size","4"},{block(0,4),"abcd"},{"/success","","+HTTPACTION: 0,500,0"}},true,1,1,1,0,2);++n;
 run({{"/size","4"},{block(0,4),"abc"}},false,1,0,0,1,1,true,true,false);++n;
 run({},false,0,0,0,0,0,true,true,true,false);++n;
 std::cout<<n<<" cellular adapter scenarios passed\n";
}
