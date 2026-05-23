#!/usr/bin/env bash
#
# Clean firmware build for the CH582M observer using the local repo toolchain
# installed by ./install.sh. Equivalent to:
#
#   make clean && make
#
# but with TOOLCHAIN_PREFIX wired to the locally-pinned GCC instead of a
# system-wide install. No environment variables required.

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

PROJECT_DIR="${PROJECT_DIR:-CH582M_SCAN}"
WORK_DIR="${WORK_DIR:-.tooling}"
RISCV_VERSION="${RISCV_VERSION:-15.2.0-1}"
XPACK_DIR="${WORK_DIR}/xpack-riscv-none-elf-gcc-${RISCV_VERSION}"
LEGACY_MRS_DIR="${WORK_DIR}/MRS_Toolchain_Linux_x64_V1.92"

if [[ -x "$(pwd)/${XPACK_DIR}/bin/riscv-none-elf-gcc" ]]; then
    TOOLCHAIN_PREFIX="$(pwd)/${XPACK_DIR}/bin/riscv-none-elf"
elif [[ -x "$(pwd)/${LEGACY_MRS_DIR}/RISC-V_Embedded_GCC12/bin/riscv-none-elf-gcc" ]]; then
    TOOLCHAIN_PREFIX="$(pwd)/${LEGACY_MRS_DIR}/RISC-V_Embedded_GCC12/bin/riscv-none-elf"
else
    TOOLCHAIN_PREFIX="$(pwd)/${XPACK_DIR}/bin/riscv-none-elf"
fi

if [[ ! -x "${TOOLCHAIN_PREFIX}-gcc" ]]; then
    echo "ERROR: repo-local RISC-V toolchain not found at ${TOOLCHAIN_PREFIX}-gcc" >&2
    echo "Run ./install.sh first." >&2
    exit 1
fi

if [[ ! -d "$PROJECT_DIR" ]]; then
    echo "ERROR: project directory not found: $PROJECT_DIR" >&2
    exit 1
fi

echo "==> Cleaning previous build artefacts"
make -C "$PROJECT_DIR" clean >/dev/null 2>&1 || true

echo "==> Building firmware with ${TOOLCHAIN_PREFIX}-gcc"
make -C "$PROJECT_DIR" \
    -j"$(nproc)" \
    TOOLCHAIN_PREFIX="${TOOLCHAIN_PREFIX}" \
    all

echo
echo "Build completed. Outputs:"
for f in observer.elf observer.bin observer.hex observer.lst observer.map observer.siz; do
    [[ -f "$PROJECT_DIR/$f" ]] && echo "  $PROJECT_DIR/$f"
done

# Print compact size summary if the toolchain provided one.
if [[ -f "$PROJECT_DIR/observer.siz" ]]; then
    echo
    echo "Size summary:"
    cat "$PROJECT_DIR/observer.siz"
fi
