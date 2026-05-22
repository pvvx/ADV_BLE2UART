#!/usr/bin/env bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

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

PROJECT_DIR="${PROJECT_DIR:-ble50_scan}"
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

echo "==> Running a clean firmware build"
rm -rf "$PROJECT_DIR_PATH/build"
"$IDF_PYTHON" "$ESP_IDF_DIR_PATH/tools/idf.py" \
    -C "$PROJECT_DIR_PATH" \
    -B "$PROJECT_DIR_PATH/build" \
    -G "Unix Makefiles" \
    build

if [[ -f "$PROJECT_DIR_PATH/build/bootloader/bootloader.bin" ]]; then
    cp -f "$PROJECT_DIR_PATH/build/bootloader/bootloader.bin" "$PROJECT_DIR_PATH/build/bootloader.bin"
fi

if [[ -f "$PROJECT_DIR_PATH/build/partition_table/partition-table.bin" ]]; then
    cp -f "$PROJECT_DIR_PATH/build/partition_table/partition-table.bin" "$PROJECT_DIR_PATH/build/partition-table.bin"
fi

echo
echo "Build completed. Generated files:"
echo "  $PROJECT_DIR/build/ble50_scan.bin"
echo "  $PROJECT_DIR/build/bootloader/bootloader.bin"
echo "  $PROJECT_DIR/build/partition_table/partition-table.bin"
