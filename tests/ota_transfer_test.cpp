#include "../src/OtaTransfer.h"
#include <assert.h>
#include <vector>
#include <utility>
#include <stdio.h>

using namespace OtaTransfer;
int main()
{
    uint32_t size = 0;
    assert(parseDecimal(" 8192\r\n", INT32_MAX, size) && size == 8192);
    assert(parseDecimal("0", INT32_MAX, size) && size == 0);
    assert(parseDecimal("2147483647", INT32_MAX, size) && size == INT32_MAX);
    for (const char *bad : {"", "-1", "+1", "12x", "1,2", "2147483648", "4294967296", "999999999999999999999"})
        assert(!parseDecimal(bad, INT32_MAX, size));
    uint8_t block[4] = {};
    for (uint32_t total : {1u, 4u, 8u, 9u})
    {
        std::vector<std::pair<uint32_t, size_t>> requests;
        size_t written = 0;
        auto result = transfer(total, block, sizeof(block),
            [&](uint32_t offset, uint8_t *, size_t n) -> int64_t {
                assert(n && n <= sizeof(block)); requests.emplace_back(offset, n); return n;
            },
            [&](uint8_t *, size_t n) { written += n; return n; }, [] {});
        assert(result == Complete && written == total && requests.size() == (total + 3) / 4);
        for (size_t i = 0; i < requests.size(); ++i) assert(requests[i].first == i * 4);
    }
    int reads = 0, writes = 0, feeds = 0;
    auto fetch = [&](uint32_t, uint8_t *, size_t n) -> int64_t {
        ++reads;
        if (reads == 1) return -1;
        return static_cast<int64_t>(reads == 2 ? n - 1 : n);
    };
    auto write = [&](uint8_t *, size_t n) { ++writes; return n; };
    auto feed = [&] { ++feeds; };
    assert(transfer(4, block, 4, fetch, write, feed) == Complete);
    assert(reads == 3 && writes == 1 && feeds == 4);
    reads = writes = 0;
    assert(transfer(4, block, 4, [&](uint32_t, uint8_t *, size_t) -> int64_t { ++reads; return -1; }, write, feed) == ReadFailed);
    assert(reads == 5 && writes == 0);
    assert(transfer(4, block, 4, [](uint32_t, uint8_t *, size_t n) -> int64_t { return n + 1; }, write, feed) == ReadFailed);
    assert(writes == 0);
    reads = 0;
    assert(transfer(8, block, 4, [&](uint32_t, uint8_t *, size_t n) -> int64_t { ++reads; return n; },
        [](uint8_t *, size_t n) { return n - 1; }, feed) == WriteFailed);
    assert(reads == 1);
    assert(transfer(0, block, 4, fetch, write, feed) == InvalidSize);
    assert(transfer(UINT32_MAX, block, 4, fetch, write, feed) == InvalidSize);
    assert(transfer(4, nullptr, 4, fetch, write, feed) == InvalidSize);
    assert(transfer(4, block, 0, fetch, write, feed) == InvalidSize);
    puts("OTA transfer checks passed");
}
