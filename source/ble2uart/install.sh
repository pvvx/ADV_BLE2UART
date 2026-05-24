#!/usr/bin/env bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

# Wrapper used by CI to install the pinned SDK/toolchain for sdk-v4023.
exec ./install_sdk_v4023.sh