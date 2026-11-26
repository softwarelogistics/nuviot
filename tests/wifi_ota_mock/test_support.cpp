#include "mock.h"

int bundleSetCalls = 0;
extern "C" const uint8_t _binary_x509_crt_bundle_start[] = {0, 1, 0, 1, 0, 1};

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
