#!/usr/bin/env bash
#
# Local CLI toolchain installer for the CH582M firmware (WeAct CH582M board).
#
# Installs everything needed to compile and flash from a Linux/Ubuntu host
# WITHOUT MounRiver Studio:
#   * xPack GNU RISC-V Embedded GCC (riscv-none-elf-gcc), pinned version
#   * wlink (WCH-Link CLI) for supported WCH-Link-family debug probes
#   * wchisp (open-source flasher) for direct USB ISP and UART ISP flashing
#
# Everything goes under ./.tooling/, never to a system directory. The script
# is idempotent; re-run after a network failure picks up where it stopped.
# Pass "clean" to wipe the toolchain.

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

# --- Configuration ----------------------------------------------------------

WORK_DIR="${WORK_DIR:-.tooling}"

# Stable repo-local bare-metal RISC-V toolchain.
# xPack ships the same riscv-none-elf prefix expected by the project and is
# easier to mirror reliably than the current WCH file server URL.
RISCV_VERSION="${RISCV_VERSION:-15.2.0-1}"
RISCV_TARBALL="xpack-riscv-none-elf-gcc-${RISCV_VERSION}-linux-x64.tar.gz"
RISCV_URL="${RISCV_URL:-https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v${RISCV_VERSION}/${RISCV_TARBALL}}"
RISCV_DIR="${WORK_DIR}/xpack-riscv-none-elf-gcc-${RISCV_VERSION}"
RISCV_PREFIX_REL="bin/riscv-none-elf"

# wchisp — Rust-implemented open-source flasher for WCH USB ISP.
WCHISP_VERSION="${WCHISP_VERSION:-0.3.0}"
WCHISP_URL_LINUX="${WCHISP_URL_LINUX:-https://github.com/ch32-rs/wchisp/releases/download/v${WCHISP_VERSION}/wchisp-v${WCHISP_VERSION}-linux-x64.tar.gz}"
WCHISP_URL_WINDOWS="${WCHISP_URL_WINDOWS:-https://github.com/ch32-rs/wchisp/releases/download/v${WCHISP_VERSION}/wchisp-v${WCHISP_VERSION}-win-x64.zip}"
WCHISP_DIR="${WORK_DIR}/wchisp"

# wlink - Rust-implemented CLI for WCH-Link probes.
# For CH582 specifically, probe-based flashing currently requires a compatible
# WCH-Link variant (for example WCH-LinkE). The WeAct/WCH docs for WCH-LinkW
# show the normal wired path using SWCLK/SWDIO/GND/3V3 with vendor tools; use
# wchisp serial mode only for an explicit UART-ISP wiring to the target.
# On Windows prefer the x86 build because upstream documents native driver
# support there, avoiding an extra libusb/Zadig step on many systems.
WLINK_VERSION="${WLINK_VERSION:-0.1.2}"
WLINK_URL_LINUX="${WLINK_URL_LINUX:-https://github.com/ch32-rs/wlink/releases/download/v${WLINK_VERSION}/wlink-v${WLINK_VERSION}-linux-x64.tar.gz}"
WLINK_URL_WINDOWS="${WLINK_URL_WINDOWS:-https://github.com/ch32-rs/wlink/releases/download/v${WLINK_VERSION}/wlink-v${WLINK_VERSION}-win-x86.zip}"
WLINK_DIR="${WORK_DIR}/wlink"

# --- Action: clean ----------------------------------------------------------

if [[ "${1:-}" == "clean" ]]; then
    echo "==> Removing local toolchain at $WORK_DIR/"
    rm -rf "$WORK_DIR"
    rm -rf CH582M_SCAN/obj CH582M_SCAN/observer.elf CH582M_SCAN/observer.hex \
        CH582M_SCAN/observer.bin CH582M_SCAN/observer.lst CH582M_SCAN/observer.map \
        2>/dev/null || true
    echo "done."
    exit 0
fi

mkdir -p "$WORK_DIR"

# --- Step 1: MRS RISC-V toolchain ------------------------------------------

if [[ ! -x "$RISCV_DIR/$RISCV_PREFIX_REL-gcc" ]]; then
    echo "==> Installing xPack RISC-V toolchain v${RISCV_VERSION}"
    if [[ ! -f "$WORK_DIR/$RISCV_TARBALL" ]]; then
        echo "    downloading $RISCV_URL"
        if command -v curl >/dev/null 2>&1; then
            curl -fL --retry 3 --retry-delay 5 -o "$WORK_DIR/$RISCV_TARBALL.part" "$RISCV_URL"
        elif command -v wget >/dev/null 2>&1; then
            wget --tries=3 -O "$WORK_DIR/$RISCV_TARBALL.part" "$RISCV_URL"
        else
            echo "ERROR: need curl or wget to download the toolchain" >&2
            exit 1
        fi
        mv "$WORK_DIR/$RISCV_TARBALL.part" "$WORK_DIR/$RISCV_TARBALL"
    fi
    echo "    extracting"
    tar -xzf "$WORK_DIR/$RISCV_TARBALL" -C "$WORK_DIR"
    test -x "$RISCV_DIR/$RISCV_PREFIX_REL-gcc" || {
        echo "ERROR: extracted toolchain not found at $RISCV_DIR" >&2
        ls -la "$WORK_DIR" >&2
        exit 1
    }
    chmod -R u+rwX "$RISCV_DIR"
else
    echo "==> xPack toolchain already present"
fi

# --- Step 2: wchisp (Linux + Windows binaries) -----------------------------

mkdir -p "$WCHISP_DIR/linux" "$WCHISP_DIR/windows"

if [[ ! -x "$WCHISP_DIR/linux/wchisp" ]]; then
    echo "==> Installing wchisp Linux binary"
    tmp=$(mktemp -d)
    if command -v curl >/dev/null 2>&1; then
        curl -fL -o "$tmp/wchisp.tar.gz" "$WCHISP_URL_LINUX"
    else
        wget -O "$tmp/wchisp.tar.gz" "$WCHISP_URL_LINUX"
    fi
    tar -xzf "$tmp/wchisp.tar.gz" -C "$tmp"
    bin=$(find "$tmp" -maxdepth 3 -type f -name "wchisp" -perm -u+x | head -n 1)
    if [[ -z "$bin" ]]; then
        # some releases ship without exec bit
        bin=$(find "$tmp" -maxdepth 3 -type f -name "wchisp" | head -n 1)
    fi
    cp "$bin" "$WCHISP_DIR/linux/wchisp"
    chmod +x "$WCHISP_DIR/linux/wchisp"
    rm -rf "$tmp"
else
    echo "==> wchisp Linux already present"
fi

if [[ ! -f "$WCHISP_DIR/windows/wchisp.exe" ]]; then
    echo "==> Installing wchisp Windows binary"
    tmp=$(mktemp -d)
    if command -v curl >/dev/null 2>&1; then
        curl -fL -o "$tmp/wchisp.zip" "$WCHISP_URL_WINDOWS"
    else
        wget -O "$tmp/wchisp.zip" "$WCHISP_URL_WINDOWS"
    fi
    unzip -q -o "$tmp/wchisp.zip" -d "$tmp"
    exe=$(find "$tmp" -maxdepth 3 -type f -name "wchisp.exe" | head -n 1)
    cp "$exe" "$WCHISP_DIR/windows/wchisp.exe"
    rm -rf "$tmp"
else
    echo "==> wchisp Windows already present"
fi

# --- Step 3: wlink (Linux + Windows binaries) ------------------------------

mkdir -p "$WLINK_DIR/linux" "$WLINK_DIR/windows"

if [[ ! -x "$WLINK_DIR/linux/wlink" ]]; then
    echo "==> Installing wlink Linux binary"
    tmp=$(mktemp -d)
    if command -v curl >/dev/null 2>&1; then
        curl -fL -o "$tmp/wlink.tar.gz" "$WLINK_URL_LINUX"
    else
        wget -O "$tmp/wlink.tar.gz" "$WLINK_URL_LINUX"
    fi
    tar -xzf "$tmp/wlink.tar.gz" -C "$tmp"
    bin=$(find "$tmp" -maxdepth 3 -type f -name "wlink" -perm -u+x | head -n 1)
    if [[ -z "$bin" ]]; then
        bin=$(find "$tmp" -maxdepth 3 -type f -name "wlink" | head -n 1)
    fi
    cp "$bin" "$WLINK_DIR/linux/wlink"
    chmod +x "$WLINK_DIR/linux/wlink"
    rm -rf "$tmp"
else
    echo "==> wlink Linux already present"
fi

if [[ ! -f "$WLINK_DIR/windows/wlink.exe" ]]; then
    echo "==> Installing wlink Windows binary"
    tmp=$(mktemp -d)
    if command -v curl >/dev/null 2>&1; then
        curl -fL -o "$tmp/wlink.zip" "$WLINK_URL_WINDOWS"
    else
        wget -O "$tmp/wlink.zip" "$WLINK_URL_WINDOWS"
    fi
    unzip -q -o "$tmp/wlink.zip" -d "$tmp"
    exe=$(find "$tmp" -maxdepth 3 -type f -name "wlink.exe" | head -n 1)
    cp "$exe" "$WLINK_DIR/windows/wlink.exe"
    rm -rf "$tmp"
else
    echo "==> wlink Windows already present"
fi

echo
echo "Installation complete."
echo "  Toolchain prefix: $(pwd)/$RISCV_DIR/$RISCV_PREFIX_REL"
echo "  Linux wlink    : $(pwd)/$WLINK_DIR/linux/wlink"
echo "  Windows wlink  : $(pwd)/$WLINK_DIR/windows/wlink.exe"
echo "  Linux flasher  : $(pwd)/$WCHISP_DIR/linux/wchisp"
echo "  Windows flasher: $(pwd)/$WCHISP_DIR/windows/wchisp.exe"
echo
echo "Next: ./build.sh"
