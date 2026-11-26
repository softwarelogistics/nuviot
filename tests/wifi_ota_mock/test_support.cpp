#include "mock.h"

extern "C" time_t time(time_t *value)
{
    if (value) *value = fakeNow;
    return fakeNow;
}

extern "C" void configTime(long, int, const char*, const char*, const char*)
{
    ++ntpCalls;
    if (ntpSyncOk) fakeNow = static_cast<time_t>(1795651200);
}
