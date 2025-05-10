#!/usr/bin/env bash

# Tested with TLSR8258/8253 Bluetooth LE Multi Connection V4.0.2.1
# https://wiki.telink-semi.cn/wiki/chip-series/TLSR825x-Series/
# https://github.com/telink-semi/tc_ble_sdk/releases/tag/V4.0.2.1
# https://github.com/telink-semi/tc_ble_sdk/archive/refs/tags/V4.0.2.1.zip

TEL_PATH=./SDK

set -euo pipefail

if [ -d "${TEL_PATH}" ]; then
    echo "Directory "${TEL_PATH}" exists. Remove or rename it to run this SDK installation procedure."
    exit 1
fi

TEMPD="$(mktemp -d)"
echo "Download SDK to '${TEMPD}'"
wget -P "${TEMPD}" https://github.com/telink-semi/tc_ble_sdk/archive/refs/tags/V4.0.2.1.zip
unzip "${TEMPD}/V4.0.2.1.zip" -d "${TEMPD}"

echo "Install updated SDK to the target directory"
mv "${TEMPD}/tc_ble_sdk-4.0.2.1/tc_ble_sdk" "${TEL_PATH}"

# Add doc (with patches)
mv "${TEMPD}/tc_ble_sdk-4.0.2.1/doc" "${TEL_PATH}"

# Remove temporary directories
rm -r "${TEMPD}/tc_ble_sdk-4.0.2.1"
rm "${TEMPD}/V4.0.2.1.zip"

echo -e "\nSDK installation completed.\n"
exit 0
