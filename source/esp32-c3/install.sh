#!/usr/bin/env bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

ESP_IDF_VERSION="${ESP_IDF_VERSION:-v6.0.1}"
ESP_IDF_SERIES="$(printf '%s' "${ESP_IDF_VERSION#v}" | cut -d. -f1,2)"
WORK_DIR="${WORK_DIR:-.tooling}"
ESP_IDF_DIR="${ESP_IDF_DIR:-$WORK_DIR/esp-idf-$ESP_IDF_VERSION}"
ESP_TOOLS_DIR="${ESP_TOOLS_DIR:-$WORK_DIR/espressif}"
ESP_IDF_JOBS="${ESP_IDF_JOBS:-8}"
PYTHON_BIN="${PYTHON_BIN:-python3}"
PROJECT_DIR="${PROJECT_DIR:-ble50_scan}"
PYTHON_VERSION="$($PYTHON_BIN - <<'PY'
import sys
print(f"{sys.version_info.major}.{sys.version_info.minor}")
PY
)"

export IDF_TOOLS_PATH="$ESP_TOOLS_DIR"
export IDF_PYTHON_ENV_PATH="${IDF_PYTHON_ENV_PATH:-$ESP_TOOLS_DIR/python_env/idf${ESP_IDF_SERIES}_py${PYTHON_VERSION}_env}"

usage() {
    cat <<'EOF'
Usage:
  ./install.sh        Install prerequisites and the local ESP-IDF toolchain
  ./install.sh clean  Remove the local toolchain and clean generated build files
EOF
}

clean_workspace() {
    echo "==> Cleaning local ESP-IDF workspace"
    rm -rf "$WORK_DIR"
    rm -rf "$PROJECT_DIR/build"
    rm -rf "$PROJECT_DIR/managed_components"
    rm -f "$PROJECT_DIR/dependencies.lock"
    rm -f "$PROJECT_DIR/sdkconfig.old"
    echo "Clean completed."
}

case "${1:-}" in
    "")
        ;;
    clean)
        clean_workspace
        exit 0
        ;;
    -h|--help|help)
        usage
        exit 0
        ;;
    *)
        usage >&2
        exit 1
        ;;
esac

if ! command -v sudo >/dev/null 2>&1; then
    echo "sudo is required to install Ubuntu prerequisites." >&2
    exit 1
fi

if [[ ! -d "$PROJECT_DIR" ]]; then
    echo "Project directory not found: $PROJECT_DIR" >&2
    exit 1
fi

echo "==> Installing prerequisites"
sudo apt-get update
sudo apt-get install -y \
    git wget flex bison gperf build-essential pkg-config cmake ninja-build ccache \
    python3 python3-pip python3-venv python3-virtualenv python3-setuptools \
    libffi-dev libssl-dev dfu-util libusb-1.0-0

echo "==> Preparing local ESP-IDF in $ESP_IDF_DIR"
mkdir -p "$WORK_DIR"

if [[ ! -d "$ESP_IDF_DIR/.git" ]]; then
    git clone --branch "$ESP_IDF_VERSION" --depth 1 https://github.com/espressif/esp-idf.git "$ESP_IDF_DIR"
else
    git -C "$ESP_IDF_DIR" fetch --depth 1 origin "refs/tags/$ESP_IDF_VERSION:refs/tags/$ESP_IDF_VERSION" || true
    git -C "$ESP_IDF_DIR" checkout "$ESP_IDF_VERSION"
fi

git -C "$ESP_IDF_DIR" submodule sync --recursive
git -C "$ESP_IDF_DIR" -c protocol.version=2 submodule update --init --recursive --depth 1 --jobs "$ESP_IDF_JOBS"

echo "==> Installing ESP32-C3 toolchain"
"$PYTHON_BIN" "$ESP_IDF_DIR/tools/idf_tools.py" install --targets=esp32c3

if [[ ! -x "$IDF_PYTHON_ENV_PATH/bin/python" ]]; then
    echo "==> Creating ESP-IDF Python environment"
    rm -rf "$IDF_PYTHON_ENV_PATH"
    if "$PYTHON_BIN" -m venv --help >/dev/null 2>&1; then
        "$PYTHON_BIN" -m venv "$IDF_PYTHON_ENV_PATH"
    else
        "$PYTHON_BIN" -m virtualenv "$IDF_PYTHON_ENV_PATH"
    fi
fi

echo "==> Installing ESP-IDF Python packages"
"$PYTHON_BIN" "$ESP_IDF_DIR/tools/idf_tools.py" install-python-env --features=core

cat <<EOF

Installation completed.

Repository root: .
ESP-IDF directory: $ESP_IDF_DIR
Tools directory: $IDF_TOOLS_PATH
Python environment: $IDF_PYTHON_ENV_PATH

Next steps:
  ./build.sh

EOF
