#include "mock.h"
#include "OtaServices.h"
#include <iostream>
std::deque<Response> replies; int opens,closes,getCount; UpdateMock Update;esp_partition_t partition{16384};bool hasPartition=true;
const std::string url="http://firmware.nuviot.com:14236/api/firmware/download/test-id";
void reset(){replies.clear();opens=closes=getCount=0;Update=UpdateMock{};hasPartition=true;partition.size=16384;}
void size(std::string s,int declared=-2,int status=200){replies.emplace_back(url+"/size",status,declared==-2?static_cast<int>(s.size()):declared,s);}
void block(int offset,int len,int status=200,int actual=-1,int declared=-1){replies.emplace_back(url+"?start="+std::to_string(offset)+"&length="+std::to_string(len),status,declared==-1?len:declared,std::string(actual==-1?len:actual,'x'));}
void terminal(bool ok,int status=200){replies.emplace_back(url+(ok?"/success":"/failed"),status,0,"");}
void run(bool success,int begins,int writes,int ends,int aborts){Console c;Hal h;NuvIoTState s;
#ifdef LCD_DISPLAY
 Display d;OtaServices ota(&d,&c,&s,&h);
#else
 OtaServices ota(&c,&s,&h);
#endif
 assert(ota.downloadOverWiFi(String(url.c_str()))==success);assert(replies.empty());assert(opens==closes);assert(h.restarts==1&&h.feeds>0&&h.watchdogs==1);assert(Update.begins==begins&&Update.writes==writes&&Update.ends==ends&&Update.aborts==aborts);for(auto &log:c.logs)assert(log.find("test-id")==std::string::npos);
}
int main(){int count=0;
 reset();size("8192");block(0,4096);block(4096,4096);terminal(true);run(true,1,2,1,0);++count;
 reset();size("4097",-1);block(0,4096);block(4096,1);terminal(true);run(true,1,2,1,0);++count;
 for(auto b:{"0","-1","2147483648","16385","abc"}){reset();size(b);terminal(false);run(false,0,0,0,0);++count;}
 reset();size("4096",6);terminal(false);run(false,0,0,0,0);++count;
 reset();size(std::string(33,'1'),-1);terminal(false);run(false,0,0,0,0);++count;
 reset();size(std::string("1\0",2));terminal(false);run(false,0,0,0,0);++count;
 reset();size("4096",-2,-1);terminal(false);run(false,0,0,0,0);++count;
 reset();size("4096");hasPartition=false;terminal(false);run(false,0,0,0,0);++count;
 reset();size("4096");Update.beginOk=false;terminal(false);run(false,1,0,0,0);++count;
 reset();size("4096");block(0,4096,-56);block(0,4096,200,2);block(0,4096,200,4097);block(0,4096,200,-1,4095);block(0,4096);terminal(true);run(true,1,1,1,0);++count;
 reset();size("4096");for(int i=0;i<5;++i)block(0,4096,404);terminal(false);run(false,1,0,0,1);++count;
 reset();size("8192");block(0,4096);Update.writeOk=false;terminal(false);run(false,1,1,0,1);++count;
 reset();size("4096");block(0,4096);Update.finishOk=false;terminal(false);run(false,1,1,0,1);++count;
 reset();size("4096");block(0,4096);Update.endOk=false;terminal(false);run(false,1,1,1,1);++count;
 reset();size("4096");block(0,4096);terminal(true,500);run(true,1,1,1,0);++count;
#ifdef CELLULAR_ENABLED
 reset();Console c;Hal h;NuvIoTState s;SIMModem m;
 #ifdef LCD_DISPLAY
 Display d;OtaServices ota(&d,&c,&s,&m,&h);
 #else
 OtaServices ota(&c,&s,&m,&h);
 #endif
 assert(ota.start(String(url.c_str()))&&m.calls==1&&opens==0);++count;
#endif
 std::cout<<count<<" mock adapter scenarios passed\n";
}
