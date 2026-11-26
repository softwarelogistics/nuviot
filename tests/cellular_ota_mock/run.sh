#!/bin/sh
# Compile selected unmodified production method bodies, not the entire SDK.
# Channel, sendCommand, connection, flash and HAL are controlled doubles.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
python3 - "$root/src/SIMModem.cpp" "$tmp/modem_excerpt.cpp" "$tmp/https_modem_excerpt.cpp" <<'PY'
import pathlib, sys
source = pathlib.Path(sys.argv[1]).read_text()
ranges = [
    ("bool SIMModem::parseSecureUrl", "bool SIMModem::setupHttpContext"),
    ("int64_t SIMModem::downloadContent", "String SIMModem::httpGetSetError"),
    ("bool SIMModem::beginDownload", "void SIMModem::hardwareReset"),
]
pathlib.Path(sys.argv[2]).write_text("".join(source[source.index(a):source.index(b, source.index(a))] for a,b in ranges))
https = ranges[0]
pathlib.Path(sys.argv[3]).write_text(source[source.index(https[0]):source.index(https[1], source.index(https[0]))])
PY
for flags in '' '-DLCD_DISPLAY=1'; do
    "${CXX:-g++}" -std=c++11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer $flags -I"$root/src" -I"$tmp" "$root/tests/cellular_ota_mock/test.cpp" -o "$tmp/test"
    "$tmp/test"
    "${CXX:-g++}" -std=c++11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer $flags -I"$root/src" -I"$tmp" "$root/tests/cellular_ota_mock/https_test.cpp" -o "$tmp/https-test"
    "$tmp/https-test"
done
