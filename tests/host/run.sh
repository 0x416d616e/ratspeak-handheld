#!/bin/sh
# Builds and runs the host tests with the system C++ compiler, once for the
# SX1262 boards and once for the M9 (LR1110, -DRSM9), whose bandwidth list differs.
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
out=${TMPDIR:-/tmp}/ratspeak-host-tests
mkdir -p "$out"
cxx=${CXX:-c++}
for variant in sx1262 m9; do
    flags=""
    [ "$variant" = m9 ] && flags="-DRSM9"
    $cxx -std=c++17 -Wall -Wextra -Werror $flags -I"$root/src/core" \
        "$root/tests/host/radio_policy_test.cpp" "$root/src/core/radio/RadioPolicy.cpp" \
        -o "$out/radio_policy_test_$variant"
    echo "== radio policy ($variant)"
    "$out/radio_policy_test_$variant"
done
