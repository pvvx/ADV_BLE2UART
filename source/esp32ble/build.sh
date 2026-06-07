#!/usr/bin/env bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

PROJECT_DIR="${PROJECT_DIR:-ble50_scan}"

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

usage() {
    cat <<'EOF'
Usage:  ./build.sh [--device <target>] [--config <board>] [--incremental]
        ./build.sh --help

Options:
  --device <target>    Target chip (esp32c3, esp32c6, esp32h2, ...).
                       Default: esp32c3 (or $ESP_TARGET env var).
  --config <board>     Board configuration (supermini-c3, esp32-c6-gpio15, ...).
                       Default: supermini-c3 (or $BOARD_CONFIG env var).
  --incremental, -i    Skip clean build; recompile only changed files (faster).
  --help               Show this help message.

Available board configs:
EOF
    list_board_configs
    cat <<'EOF'
  (add your own: boards/<name>.conf)
EOF
}

# Parse arguments
ESP_TARGET="${ESP_TARGET:-}"
BOARD_CONFIG="${BOARD_CONFIG:-}"
INCREMENTAL=false
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
        --incremental|-i)
            INCREMENTAL=true
            ;;
        --help|-h)
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

resolve_path() {
    case "$1" in
        /*)
            printf '%s\n' "$1"
            ;;
        *)
            printf '%s/%s\n' "$PWD" "$1"
            ;;
    esac
}

ESP_IDF_VERSION="${ESP_IDF_VERSION:-v6.0.1}"
ESP_IDF_SERIES="$(printf '%s' "${ESP_IDF_VERSION#v}" | cut -d. -f1,2)"
WORK_DIR="${WORK_DIR:-.tooling}"
ESP_IDF_DIR="${ESP_IDF_DIR:-$WORK_DIR/esp-idf-$ESP_IDF_VERSION}"
ESP_TOOLS_DIR="${ESP_TOOLS_DIR:-$WORK_DIR/espressif}"
PYTHON_BIN="${PYTHON_BIN:-python3}"
PYTHON_VERSION="$($PYTHON_BIN - <<'PY'
import sys
print(f"{sys.version_info.major}.{sys.version_info.minor}")
PY
)"
PROJECT_DIR_PATH="$(resolve_path "$PROJECT_DIR")"
ESP_IDF_DIR_PATH="$(resolve_path "$ESP_IDF_DIR")"
ESP_TOOLS_DIR_PATH="$(resolve_path "$ESP_TOOLS_DIR")"
IDF_PYTHON_ENV_PATH_DEFAULT="$ESP_TOOLS_DIR_PATH/python_env/idf${ESP_IDF_SERIES}_py${PYTHON_VERSION}_env"

export IDF_PATH="$ESP_IDF_DIR_PATH"
export IDF_TOOLS_PATH="$ESP_TOOLS_DIR_PATH"
export IDF_PYTHON_ENV_PATH="${IDF_PYTHON_ENV_PATH:-$IDF_PYTHON_ENV_PATH_DEFAULT}"
export ESP_IDF_VERSION="${ESP_IDF_VERSION#v}"

IDF_PYTHON="$IDF_PYTHON_ENV_PATH/bin/python"
RISCV_GCC="$(find "$ESP_TOOLS_DIR_PATH/tools/riscv32-esp-elf" -type f -name 'riscv32-esp-elf-gcc' 2>/dev/null | sort | head -n 1)"
RISCV_GXX="$(find "$ESP_TOOLS_DIR_PATH/tools/riscv32-esp-elf" -type f -name 'riscv32-esp-elf-g++' 2>/dev/null | sort | head -n 1)"
RISCV_TOOL_BIN="$(dirname "$RISCV_GCC")"

if [[ ! -d "$PROJECT_DIR_PATH" ]]; then
    echo "Project directory not found: $PROJECT_DIR" >&2
    exit 1
fi

if [[ ! -d "$ESP_IDF_DIR_PATH" ]]; then
    echo "ESP-IDF directory not found: $ESP_IDF_DIR" >&2
    echo "Run ./install.sh first." >&2
    exit 1
fi

if [[ ! -x "$IDF_PYTHON" ]]; then
    echo "ESP-IDF Python environment not found: $IDF_PYTHON" >&2
    echo "Run ./install.sh first." >&2
    exit 1
fi

if [[ -z "$RISCV_GCC" || -z "$RISCV_GXX" || ! -x "$RISCV_GCC" || ! -x "$RISCV_GXX" ]]; then
    echo "RISC-V ESP toolchain not found under $ESP_TOOLS_DIR/tools" >&2
    echo "Run ./install.sh first." >&2
    exit 1
fi

export PATH="$RISCV_TOOL_BIN:$PATH"
export CC="$RISCV_GCC"
export CXX="$RISCV_GXX"
export ASM="$RISCV_GCC"
export CMAKE_C_COMPILER="$RISCV_GCC"
export CMAKE_CXX_COMPILER="$RISCV_GXX"
export CMAKE_ASM_COMPILER="$RISCV_GCC"

# Map board config to config file
BOARD_CONFIG_FILE="$PROJECT_DIR_PATH/boards/${BOARD_CONFIG}.conf"
if [[ ! -f "$BOARD_CONFIG_FILE" ]]; then
    echo "Board configuration not found: $BOARD_CONFIG_FILE" >&2
    echo "Available board configs:" >&2
    list_board_configs >&2
    exit 1
fi

# SDKCONFIG_DEFAULTS: base settings + board-specific overrides
export SDKCONFIG_DEFAULTS="sdkconfig.defaults;boards/${BOARD_CONFIG}.conf"

echo "==> Target: $ESP_TARGET, Board: $BOARD_CONFIG"
# Detect current target from sdkconfig
SDKCONFIG="$PROJECT_DIR_PATH/sdkconfig"
CURRENT_TARGET=""
if [[ -f "$SDKCONFIG" ]]; then
    CURRENT_TARGET="$(grep '^CONFIG_IDF_TARGET=' "$SDKCONFIG" | sed 's/^CONFIG_IDF_TARGET="//;s/"$//')"
fi

NEED_SET_TARGET=false

# First build (no sdkconfig yet): run set-target to apply board config
if [[ ! -f "$SDKCONFIG" ]]; then
    echo "==> No sdkconfig found — running set-target for fresh configuration"
    NEED_SET_TARGET=true
fi

# If target changed, run set-target first
if [[ -n "$CURRENT_TARGET" && "$CURRENT_TARGET" != "$ESP_TARGET" ]]; then
    echo "==> sdkconfig was generated for '$CURRENT_TARGET', switching to '$ESP_TARGET'"
    NEED_SET_TARGET=true
fi

# Content-hash of the board config file: saves the md5sum after a successful build
# and compares it on subsequent runs.  This catches renames, edits, and any change
# that timestamp -nt alone would miss (e.g. file edited then reverted, or renamed
# while preserving mtime).
BOARD_CONFIG_HASH_FILE="$PROJECT_DIR_PATH/build/.board_config_hash"
if [[ -f "$SDKCONFIG" ]]; then
    CURRENT_HASH="$(md5sum "$BOARD_CONFIG_FILE" 2>/dev/null | awk '{print $1}')"
    SAVED_HASH="$(cat "$BOARD_CONFIG_HASH_FILE" 2>/dev/null || true)"
    if [[ -n "$CURRENT_HASH" && "$CURRENT_HASH" != "$SAVED_HASH" ]]; then
        echo "==> Board config '$BOARD_CONFIG.conf' content changed, re-running set-target"
        NEED_SET_TARGET=true
    fi
fi

# Legacy timestamp check: if board config file is newer than sdkconfig, regenerate
if [[ -f "$SDKCONFIG" && "$BOARD_CONFIG_FILE" -nt "$SDKCONFIG" ]]; then
    echo "==> Board config '$BOARD_CONFIG.conf' is newer than sdkconfig, re-running set-target"
    NEED_SET_TARGET=true
fi

if [ "$NEED_SET_TARGET" = true ]; then
    echo "==> Removing stale sdkconfig to force fresh generation from defaults"
    rm -f "$SDKCONFIG"
    rm -f "${SDKCONFIG}.old"
    rm -rf "$PROJECT_DIR_PATH/build"
    export IDF_TARGET="$ESP_TARGET"
    "$IDF_PYTHON" "$ESP_IDF_DIR_PATH/tools/idf.py" \
        -C "$PROJECT_DIR_PATH" \
        -B "$PROJECT_DIR_PATH/build" \
        set-target "$ESP_TARGET"
fi

BUILD_TYPE="clean"
if [ "$INCREMENTAL" = true ]; then
    BUILD_TYPE="incremental"
fi
echo "==> Running a $BUILD_TYPE firmware build"
if [ "$INCREMENTAL" != true ]; then
    rm -rf "$PROJECT_DIR_PATH/build"
fi
export IDF_TARGET="$ESP_TARGET"
"$IDF_PYTHON" "$ESP_IDF_DIR_PATH/tools/idf.py" \
    -C "$PROJECT_DIR_PATH" \
    -B "$PROJECT_DIR_PATH/build" \
    build

# Save board config hash for future content-change detection
mkdir -p "$(dirname "$BOARD_CONFIG_HASH_FILE")"
md5sum "$BOARD_CONFIG_FILE" 2>/dev/null | awk '{print $1}' > "$BOARD_CONFIG_HASH_FILE" || true

echo
echo "Build completed. Generated files:"
echo "  $PROJECT_DIR/build/ble50_scan.bin"
echo "  $PROJECT_DIR/build/bootloader/bootloader.bin"
echo "  $PROJECT_DIR/build/partition_table/partition-table.bin"
echo "Target chip: $ESP_TARGET"
