#!/usr/bin/env bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

PROJECT_DIR="${PROJECT_DIR:-ble50_scan}"

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
Usage:  ./build.sh [--device <target>] [--led <gpio>] [--rgb <gpio>] [--adv <mode>] [--incremental]
        ./build.sh --help

Options:
  --device <target>    Target chip (esp32c3, esp32c6, esp32h2, ...).
                       Default: esp32c3 (or $ESP_TARGET env var).
  --led <gpio>         Regular LED on this GPIO (e.g. --led 8).
  --rgb <gpio>         RGB LED (SK6812) on this GPIO (e.g. --rgb 15).
  --adv <mode>         What blinks on received advertisements:
                       led  — regular LED (default GPIO 8),
                       rgb  — RGB LED (default GPIO 15),
                       none — no blink.
  --incremental, -i    Skip clean build; recompile only changed files (faster).
  --help               Show this help message.
EOF
}

# Parse arguments
ESP_TARGET="${ESP_TARGET:-}"
INCREMENTAL=false
LED_GPIO="${LED_GPIO:-}"
RGB_GPIO="${RGB_GPIO:-}"
ADV_MODE="${ADV_MODE:-}"
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

# ── Detect target architecture and locate the correct toolchain ───────────
# RISC-V chips: esp32c2/c3/c5/c6/c61, esp32h2/h4/h21, esp32p4, esp32s31, esp32e22
# Xtensa chips: esp32, esp32s2, esp32s3 (and esp8266 via old SDK, unsupported here)
#
is_riscv_target() {
    local t="$1"
    case "$t" in
        esp32c2|esp32c3|esp32c5|esp32c6|esp32c61|esp32h2|esp32h4|esp32h21|esp32p4|esp32s31|esp32e22) return 0 ;;
        *) return 1 ;;
    esac
}

TOOLCHAIN_GCC=""
TOOLCHAIN_GXX=""
TOOLCHAIN_BIN=""

if is_riscv_target "$ESP_TARGET"; then
    TOOLCHAIN_GCC="$(find "$ESP_TOOLS_DIR_PATH/tools/riscv32-esp-elf" -type f -name 'riscv32-esp-elf-gcc' 2>/dev/null | sort | head -n 1)"
    TOOLCHAIN_GXX="$(find "$ESP_TOOLS_DIR_PATH/tools/riscv32-esp-elf" -type f -name 'riscv32-esp-elf-g++' 2>/dev/null | sort | head -n 1)"
else
    # Xtensa targets: esp32, esp32s2, esp32s3
    # All Xtensa chips share a single toolchain directory (xtensa-esp-elf).
    TOOLCHAIN_GCC="$(find "$ESP_TOOLS_DIR_PATH/tools/xtensa-esp-elf" -type f -name "xtensa-${ESP_TARGET}-elf-gcc" 2>/dev/null | sort | head -n 1)"
    TOOLCHAIN_GXX="$(find "$ESP_TOOLS_DIR_PATH/tools/xtensa-esp-elf" -type f -name "xtensa-${ESP_TARGET}-elf-g++" 2>/dev/null | sort | head -n 1)"
fi
TOOLCHAIN_BIN="$(dirname "$TOOLCHAIN_GCC")"

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

if [[ -z "$TOOLCHAIN_GCC" || -z "$TOOLCHAIN_GXX" || ! -x "$TOOLCHAIN_GCC" || ! -x "$TOOLCHAIN_GXX" ]]; then
    echo "Toolchain not found for target '$ESP_TARGET' under $ESP_TOOLS_DIR/tools" >&2
    if is_riscv_target "$ESP_TARGET"; then
        echo "Expected RISC-V toolchain (riscv32-esp-elf). Run ./install.sh first." >&2
    else
        echo "Expected Xtensa toolchain (xtensa-${ESP_TARGET#esp}-elf). Run ./install.sh first." >&2
    fi
    exit 1
fi

export PATH="$TOOLCHAIN_BIN:$PATH"
export CC="$TOOLCHAIN_GCC"
export CXX="$TOOLCHAIN_GXX"
export ASM="$TOOLCHAIN_GCC"
export CMAKE_C_COMPILER="$TOOLCHAIN_GCC"
export CMAKE_CXX_COMPILER="$TOOLCHAIN_GXX"
export CMAKE_ASM_COMPILER="$TOOLCHAIN_GCC"

# ── Generate board config fragment from --led / --rgb / --adv flags ──────────
#
# Determine the primary "board LED" (the one used for activity indication).
# Logic:
#   - If only --led is given (no --rgb):   primary = PWM LED on that GPIO.
#   - If only --rgb is given (no --led):   primary = RGB LED on that GPIO.
#   - If both --led and --rgb are given:
#       adv=led → primary = PWM LED on --led GPIO,
#       adv=rgb → primary = RGB LED on --rgb GPIO,
#       adv=none or unset → primary = PWM LED on --led GPIO (default).
#   - If only --adv is given (no --led, no --rgb):
#       adv=led → PWM LED on default GPIO 8,
#       adv=rgb → RGB LED on default GPIO 15 via RMT,
#       adv=none → no board LED.
#   - If no flags at all: no board LED.
#
BOARD_LED_GPIO=""
BOARD_LED_TYPE=""         # "pwm" or "rgb"
GEN_BOARD_CONF="$PROJECT_DIR_PATH/gen_board_config.conf"

resolve_board_config() {
    local led_gpio="$1"
    local rgb_gpio="$2"
    local adv="$3"

    # No default GPIOs: if --adv led/rgb is specified, the corresponding
    # --led/--rgb must have been provided (validated above). If neither
    # --led nor --rgb is set, BOARD_LED_GPIO stays empty (no LED).

    if [[ -n "$led_gpio" && -n "$rgb_gpio" ]]; then
        # Both LEDs present — pick primary based on --adv
        case "$adv" in
            rgb) BOARD_LED_GPIO="$rgb_gpio"; BOARD_LED_TYPE="rgb" ;;
            none) BOARD_LED_GPIO=""; BOARD_LED_TYPE="" ;;
            *) BOARD_LED_GPIO="$led_gpio"; BOARD_LED_TYPE="pwm" ;;
        esac
    elif [[ -n "$rgb_gpio" ]]; then
        # Only RGB
        BOARD_LED_GPIO="$rgb_gpio"
        BOARD_LED_TYPE="rgb"
    elif [[ -n "$led_gpio" ]]; then
        # Only regular LED
        BOARD_LED_GPIO="$led_gpio"
        BOARD_LED_TYPE="pwm"
    fi
}

resolve_board_config "$LED_GPIO" "$RGB_GPIO" "$ADV_MODE"

# Derive a single-byte HW_VERSION from the target name.
# Convention: take the last digit(s) of the chip identifier and prefix with
# a series byte (0xC for 'c', 0x3 for 's', 0xE for 'h', 0x5 for 'p', etc.).
# If no mapping applies, default to 0x00.
derive_hw_version() {
    local target="$1"
    local chip="${target#esp}"
    # Handle esp8266 / plain esp32
    case "$chip" in
        8266) echo "0x00"; return ;;
        32)   echo "0x00"; return ;;
    esac
    # For named series (esp32c3, esp32s3, esp32h2, esp32p4 …):
    #   chip = "32c3", "32s3", "32h2", "32p4", "32c61" …
    #   series = "${chip:2:1}"   → "c", "s", "h", "p", "c"
    #   number = "${chip:3}"     → "3", "3", "2", "4", "61"
    local series="${chip:2:1}"
    local number="${chip:3}"
    local prefix
    case "$series" in
        c) prefix="C" ;;
        s) prefix="3" ;;
        h) prefix="E" ;;
        p) prefix="5" ;;
        e) prefix="E" ;;   # esp32e22
        *) echo "0x00"; return ;;
    esac
    # number must be valid hex; combine prefix (high nibble) with the
    # last hex digit of number (low nibble) to get a single byte.
    if [[ "$number" =~ ^[0-9A-Fa-f]+$ ]]; then
        local last="${number: -1}"
        printf '0x%s%s\n' "$prefix" "$last"
    else
        echo "0x00"
    fi
}
HW_VERSION="$(derive_hw_version "$ESP_TARGET")"

# Write the generated board config fragment (new Kconfig symbols)
mkdir -p "$(dirname "$GEN_BOARD_CONF")"
{
    echo "# Auto-generated board configuration"
    echo "# LED_GPIO=$LED_GPIO RGB_GPIO=$RGB_GPIO ADV_MODE=$ADV_MODE"
    echo "CONFIG_BOARD_HW_VERSION=$HW_VERSION"
    echo "CONFIG_BOARD_LED_PULSE_US=25000"
    # Regular (PWM) LED
    if [[ -n "$LED_GPIO" ]]; then
        echo "CONFIG_LED_GPIO=$LED_GPIO"
        echo "CONFIG_LED_ACTIVE_LOW=y"
        echo "CONFIG_LED_IDLE_LEVEL=1"
        echo "CONFIG_BOARD_GPIO_${LED_GPIO}=y"
    else
        echo "CONFIG_LED_GPIO=0"
    fi
    # RGB LED via RMT
    if [[ -n "$RGB_GPIO" ]]; then
        echo "CONFIG_RGB_GPIO=$RGB_GPIO"
        echo "CONFIG_BOARD_GPIO_${RGB_GPIO}=y"
    else
        echo "CONFIG_RGB_GPIO=0"
    fi
    # Advertisement blink selection
    case "$ADV_MODE" in
        led) echo "CONFIG_ADV_BLINK_LED=y" ;;
        rgb) echo "CONFIG_ADV_BLINK_RGB=y" ;;
        *)   echo "CONFIG_ADV_BLINK_NONE=y" ;;
    esac
} > "$GEN_BOARD_CONF"

# SDKCONFIG_DEFAULTS: base settings + generated board config
export SDKCONFIG_DEFAULTS="sdkconfig.defaults;gen_board_config.conf"

echo "==> Target: $ESP_TARGET  LED: ${LED_GPIO:-none}  RGB: ${RGB_GPIO:-none}  ADV: ${ADV_MODE:-none}  HW: $HW_VERSION"

# Detect current target from sdkconfig
SDKCONFIG="$PROJECT_DIR_PATH/sdkconfig"
CURRENT_TARGET=""
if [[ -f "$SDKCONFIG" ]]; then
    CURRENT_TARGET="$(grep '^CONFIG_IDF_TARGET=' "$SDKCONFIG" | sed 's/^CONFIG_IDF_TARGET="//;s/"$//')"
fi

NEED_SET_TARGET=false

# First build (no sdkconfig yet): run set-target to apply config
if [[ ! -f "$SDKCONFIG" ]]; then
    echo "==> No sdkconfig found — running set-target for fresh configuration"
    NEED_SET_TARGET=true
fi

# If target changed, run set-target first
if [[ -n "$CURRENT_TARGET" && "$CURRENT_TARGET" != "$ESP_TARGET" ]]; then
    echo "==> sdkconfig was generated for '$CURRENT_TARGET', switching to '$ESP_TARGET'"
    NEED_SET_TARGET=true
fi

# Content-hash of generated board config: triggers set-target on change
GEN_BOARD_CONF_HASH_FILE="$PROJECT_DIR_PATH/build/.gen_board_config_hash"
if [[ -f "$SDKCONFIG" ]]; then
    CURRENT_HASH="$(md5sum "$GEN_BOARD_CONF" 2>/dev/null | awk '{print $1}')"
    SAVED_HASH="$(cat "$GEN_BOARD_CONF_HASH_FILE" 2>/dev/null || true)"
    if [[ -n "$CURRENT_HASH" && "$CURRENT_HASH" != "$SAVED_HASH" ]]; then
        echo "==> Board configuration changed, re-running set-target"
        NEED_SET_TARGET=true
    fi
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

# Save generated config hash for future content-change detection
mkdir -p "$(dirname "$GEN_BOARD_CONF_HASH_FILE")"
md5sum "$GEN_BOARD_CONF" 2>/dev/null | awk '{print $1}' > "$GEN_BOARD_CONF_HASH_FILE" || true

echo
echo "Build completed. Generated files:"
echo "  $PROJECT_DIR/build/ble50_scan.bin"
echo "  $PROJECT_DIR/build/bootloader/bootloader.bin"
echo "  $PROJECT_DIR/build/partition_table/partition-table.bin"
echo "Target chip: $ESP_TARGET"
