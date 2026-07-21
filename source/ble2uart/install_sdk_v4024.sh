#!/usr/bin/env bash

# Installation script for Telink BLE SDK V4.0.2.3 from GitHub.
# The SDK repository stores the actual SDK under tc_ble_sdk/; this script
# installs that content directly into ./SDK without keeping a nested .git repo.

TEL_PATH=./SDK
SDK_TAG=V4.0.2.4
SDK_REPO=https://github.com/telink-semi/tc_ble_sdk.git
USE_LEGACY_LIBRARY=yes

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

if [ "$USE_LEGACY_LIBRARY" == yes ]; then
    # liblt_8258.a in V4.0.2.2, V4.0.2.3 and V4.0.2.4 has a bug that crashes the BLE stack
    # when receiving Coded PHY advertisements. Replace it with the V4.0.2.1 library.
    echo "Applying liblt_8258.a fix: replacing with V4.0.2.1 library (Coded PHY bug workaround)..."
    LIB_FIX_TAG=V4.0.2.1
    git clone --depth 1 --branch "${LIB_FIX_TAG}" "${SDK_REPO}" "${TEMPD}/tc_ble_sdk_fix"
    cp "${TEMPD}/tc_ble_sdk_fix/tc_ble_sdk/proj_lib/liblt_8258.a" "${TEL_PATH}/proj_lib/liblt_8258.a"
    echo "liblt_8258.a replaced with ${LIB_FIX_TAG} fix."
fi

echo
echo "SDK ${SDK_TAG} installation completed."
