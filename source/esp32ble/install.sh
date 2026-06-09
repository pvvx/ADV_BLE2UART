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

# Known ESP32-family targets (extensible list).
KNOWN_TARGETS=(
    esp8266 esp32 esp32s2 esp32s3 esp32c3 esp32c2 esp32c6 esp32c61
    esp32c5 esp32e22 esp32h2 esp32h21 esp32p4 esp32h4 esp32s31
)

is_valid_target() {
    local t="$1"
    for known in "${KNOWN_TARGETS[@]}"; do
        [[ "$t" == "$known" ]] && return 0
    done
    return 1
}

usage() {
    cat <<'EOF'
Usage:
  ./install.sh                     Install prerequisites and the local ESP-IDF toolchain
  ./install.sh clean               Remove the local toolchain and clean generated build files
  ./install.sh --device <target>   Install toolchain for the specified chip target
  ./install.sh --led <gpio>        Configure a regular LED on the given GPIO
  ./install.sh --rgb <gpio>        Configure an RGB LED (SK6812) on the given GPIO
  ./install.sh --adv <mode>        Advertisement blink mode: led, rgb, none

Options:
  --device <target>   Target chip (esp32c3, esp32c6, esp32h2, ...).
                      Default: esp32c3 (or $ESP_TARGET env var).
  --led <gpio>        Regular LED on this GPIO (e.g. --led 8).
  --rgb <gpio>        RGB LED (SK6812) on this GPIO (e.g. --rgb 15).
  --adv <mode>        What blinks on received advertisements:
                      led  — regular LED (default GPIO 8),
                      rgb  — RGB LED (default GPIO 15),
                      none — no blink.
  --help              Show this help message.
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

# Parse arguments
ACTION=""
LED_GPIO=""
RGB_GPIO=""
ADV_MODE=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --device)
            shift
            ESP_TARGET="${1:-}"
            ;;
        --led)
            shift
            LED_GPIO="${1:-}"
            ;;
        --rgb)
            shift
            RGB_GPIO="${1:-}"
            ;;
        --adv)
            shift
            ADV_MODE="${1:-}"
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

# Validate device target
if ! is_valid_target "$ESP_TARGET"; then
    echo "Error: invalid or unknown device target '$ESP_TARGET'." >&2
    echo "Valid targets: ${KNOWN_TARGETS[*]}" >&2
    exit 1
fi

# Get the maximum GPIO number for the target chip
get_max_gpio() {
    case "$1" in
        esp32s3) echo 48 ;;
        esp32s2) echo 46 ;;
        esp32)   echo 39 ;;
        esp32h2|esp32h4|esp32h21) echo 27 ;;
        esp32c6|esp32c61|esp32c5) echo 23 ;;
        esp32p4) echo 23 ;;
        esp32c3) echo 21 ;;
        esp32c2|esp32e22|esp32s31) echo 20 ;;
        esp8266) echo 16 ;;
        *)       echo 47 ;;
    esac
}
MAX_GPIO="$(get_max_gpio "$ESP_TARGET")"

# Validate --led
if [[ -n "$LED_GPIO" ]]; then
    if ! [[ "$LED_GPIO" =~ ^[0-9]+$ ]] || [[ "$LED_GPIO" -lt 0 ]] || [[ "$LED_GPIO" -gt "$MAX_GPIO" ]]; then
        echo "Error: --led requires a valid GPIO number (0-$MAX_GPIO) for $ESP_TARGET, got '$LED_GPIO'" >&2
        exit 1
    fi
fi

# Validate --rgb
if [[ -n "$RGB_GPIO" ]]; then
    if ! [[ "$RGB_GPIO" =~ ^[0-9]+$ ]] || [[ "$RGB_GPIO" -lt 0 ]] || [[ "$RGB_GPIO" -gt "$MAX_GPIO" ]]; then
        echo "Error: --rgb requires a valid GPIO number (0-$MAX_GPIO) for $ESP_TARGET, got '$RGB_GPIO'" >&2
        exit 1
    fi
fi

# Validate --adv: must reference an existing LED
if [[ -n "$ADV_MODE" ]]; then
    case "$ADV_MODE" in
        led)
            if [[ -z "$LED_GPIO" ]]; then
                echo "Error: --adv led requires --led <gpio> (no LED configured)" >&2
                exit 1
            fi
            ;;
        rgb)
            if [[ -z "$RGB_GPIO" ]]; then
                echo "Error: --adv rgb requires --rgb <gpio> (no RGB LED configured)" >&2
                exit 1
            fi
            ;;
        none) ;;
        *)
            echo "Error: --adv must be 'led', 'rgb', or 'none', got '$ADV_MODE'" >&2
            exit 1
            ;;
    esac
fi

# Export variables so build.sh inherits them
export LED_GPIO RGB_GPIO ADV_MODE

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
LED GPIO: ${LED_GPIO:-none}
RGB GPIO: ${RGB_GPIO:-none}
Adv blink: ${ADV_MODE:-none}

Next steps:
  ./build.sh

EOF
