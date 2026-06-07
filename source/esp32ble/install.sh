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
ESPTOOL_VERSION="${ESPTOOL_VERSION:-v5.3.0}"
ESP_TARGET="${ESP_TARGET:-}"
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
  ./install.sh                     Install prerequisites and the local ESP-IDF toolchain
  ./install.sh clean               Remove the local toolchain and clean generated build files
  ./install.sh --device <target>   Install toolchain for the specified chip target
  ./install.sh --config <board>    Select board configuration

Options:
  --device <target>   Target chip (esp32c3, esp32c6, esp32h2, ...).
                      Default: esp32c3 (or $ESP_TARGET env var).
  --config <board>    Board configuration (supermini-c3, esp32-c6-gpio15, ...).
                      Default: supermini-c3 (or $BOARD_CONFIG env var).
  --help              Show this help message.

Available board configs:
EOF
    list_board_configs
    cat <<'EOF'
  (add your own: boards/<name>.conf)
EOF
}

# List available board configurations from the boards/ directory
list_board_configs() {
    local boards_dir="$PROJECT_DIR/boards"
    if [[ -d "$boards_dir" ]]; then
        for f in "$boards_dir"/*.conf; do
            if [[ -f "$f" ]]; then
                name="$(basename "$f" .conf)"
                # Extract the second line (first comment) as a short description
                desc="$(sed -n '2p' "$f" 2>/dev/null | sed 's/^# //' || true)"
                if [[ -z "$desc" ]]; then
                    desc="Custom board configuration"
                fi
                if [[ "$name" == "${BOARD_CONFIG:-supermini-c3}" ]]; then
                    printf "  %-20s %s (default)\n" "$name" "$desc"
                else
                    printf "  %-20s %s\n" "$name" "$desc"
                fi
            fi
        done
    fi
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

# Parse arguments
ACTION=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --device)
            shift
            ESP_TARGET="${1:-}"
            ;;
        --config)
            shift
            BOARD_CONFIG="${1:-}"
            ;;
        clean)
            ACTION=clean
            ;;
        --help|-h|help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
    shift
done
: "${ESP_TARGET:=esp32c3}"
: "${BOARD_CONFIG:=supermini-c3}"

# Validate board config (skip for clean action)
if [[ "$ACTION" != "clean" ]]; then
    BOARD_CONFIG_FILE="$PROJECT_DIR/boards/${BOARD_CONFIG}.conf"
    if [[ ! -f "$BOARD_CONFIG_FILE" ]]; then
        echo "Error: Board configuration not found: $BOARD_CONFIG_FILE" >&2
        echo "Available board configs:" >&2
        list_board_configs >&2
        exit 1
    fi
fi
export BOARD_CONFIG

case "$ACTION" in
    clean)
        clean_workspace
        exit 0
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

echo "==> Installing $ESP_TARGET toolchain"
"$PYTHON_BIN" "$ESP_IDF_DIR/tools/idf_tools.py" install --targets="$ESP_TARGET"

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

echo "==> Installing esptool $ESPTOOL_VERSION (Windows)"
ESPTOOL_ZIP="esptool-$ESPTOOL_VERSION-windows-amd64.zip"
ESPTOOL_URL="https://github.com/espressif/esptool/releases/download/$ESPTOOL_VERSION/$ESPTOOL_ZIP"
ESPTOOL_TMPDIR="$(mktemp -d)"
if wget -q "$ESPTOOL_URL" -O "$ESPTOOL_TMPDIR/$ESPTOOL_ZIP"; then
    unzip -q -o "$ESPTOOL_TMPDIR/$ESPTOOL_ZIP" -d "$ESPTOOL_TMPDIR"
    cp "$ESPTOOL_TMPDIR/esptool-windows-amd64/esptool.exe" "$PROJECT_DIR/esptool.exe"
    echo "  -> esptool.exe installed in $PROJECT_DIR/"
else
    echo "  -> WARNING: could not download esptool $ESPTOOL_VERSION from GitHub."
    echo "     You can manually place esptool.exe in $PROJECT_DIR/"
fi
rm -rf "$ESPTOOL_TMPDIR"

cat <<EOF

Installation completed.

Repository root: .
ESP-IDF directory: $ESP_IDF_DIR
Tools directory: $IDF_TOOLS_PATH
Python environment: $IDF_PYTHON_ENV_PATH
esptool: $PROJECT_DIR/esptool.exe ($ESPTOOL_VERSION)
Target chip: $ESP_TARGET

Next steps:
  ./build.sh

EOF
