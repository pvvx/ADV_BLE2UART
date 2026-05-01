#!/usr/bin/env bash

# Installation script for Telink BLE SDK V4.0.2.1 from GitHub.
# The SDK repository stores the actual SDK under tc_ble_sdk/; this script
# installs that content directly into ./SDK without keeping a nested .git repo.
#
# NOTE: V4.0.2.1 is the last known good version where liblt_8258.a correctly
# handles Coded PHY advertisement reception. V4.0.2.2 and V4.0.2.3 have a bug
# that causes the BLE stack to crash/freeze on Coded PHY packets.

TEL_PATH=./SDK
SDK_TAG=V4.0.2.1
SDK_REPO=https://github.com/telink-semi/tc_ble_sdk.git

set -euo pipefail

if [ -d "${TEL_PATH}" ]; then
        echo "Directory '${TEL_PATH}' exists. Remove or rename it to run this SDK installation procedure."
        exit 1
fi

TEMPD="$(mktemp -d)"
trap 'rm -rf "${TEMPD}"' EXIT

echo "Cloning Telink BLE SDK ${SDK_TAG} from GitHub..."
git clone --depth 1 --branch "${SDK_TAG}" "${SDK_REPO}" "${TEMPD}/tc_ble_sdk"

echo "Installing SDK content to ${TEL_PATH}..."
mkdir -p "${TEL_PATH}"
cp -a "${TEMPD}/tc_ble_sdk/tc_ble_sdk/." "${TEL_PATH}/"

echo
echo "SDK ${SDK_TAG} installation completed."
