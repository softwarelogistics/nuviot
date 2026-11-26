#!/bin/sh
# Host-only adapter verification. Not an ESP32 SDK or hardware compilation.
set -eu
CXX=g++
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
fixture="$root/tests/wifi_ota_mock"
if grep -q "setInsecure" "$root/src/OtaServices.cpp"; then
    echo "unsafe TLS API found" >&2
    exit 1
fi
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp/src" "$tmp/stubs"
cp "$root/src/OtaServices.cpp" "$root/src/OtaServices.h" "$root/src/OtaTransfer.h" "$tmp/src/"
for h in Hal.h Display.h SIMModem.h NuvIoTState.h HTTPClient.h Update.h esp_spi_flash.h esp_ota_ops.h esp_image_format.h WiFiClientSecure.h mbedtls/x509_crt.h; do
    mkdir -p "$tmp/stubs/$(dirname "$h")"
    printf '#include "mock.h"\n' > "$tmp/stubs/$h"
done
"$CXX" -std=c++11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer "$root/tests/ota_transfer_test.cpp" -o "$tmp/transfer-test"
"$tmp/transfer-test"

for flags in '' '-DLCD_DISPLAY=1' '-DCELLULAR_ENABLED=1' '-DLCD_DISPLAY=1 -DCELLULAR_ENABLED=1'; do
    printf 'flags=%s\n' "$flags"
    # Intentional word splitting: flags are fixed literals above.
    "$CXX" -DARDUINO_ARCH_ESP32=1 -DCONFIG_MBEDTLS_CERTIFICATE_BUNDLE=1 -std=c++11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer $flags -I"$fixture" -I"$tmp/stubs" -I"$tmp/src" "$tmp/src/OtaServices.cpp" "$fixture/test.cpp" -o "$tmp/http-test"
    "$tmp/http-test"
    "$CXX" -std=c++11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer $flags -I"$fixture" -I"$tmp/stubs" -I"$tmp/src" "$tmp/src/OtaServices.cpp" "$fixture/test_support.cpp" "$fixture/tls_test.cpp" -o "$tmp/https-test"
    "$tmp/https-test"
done
