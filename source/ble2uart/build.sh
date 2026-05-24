#!/usr/bin/env bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

make clean
# Run pre-build serially first to ensure tc32 toolchain download/extract is
# complete before any compile job starts.
make pre-build
make -j"$(nproc)" main-build secondary-outputs