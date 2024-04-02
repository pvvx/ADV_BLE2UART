#!/usr/bin/env bash

# Tested with telink_b85m_ble_sdk_V4.0.1.3_Patch/patch_0001_20231201

TEL_PATH=./SDK

set -euo pipefail

if [ -d "${TEL_PATH}" ]; then
    echo "Directory "${TEL_PATH}" exists. Remove or rename it to run this SDK installation procedure."
    exit 1
fi

# Download SDK to "${TEMPD}"
TEMPD="$(mktemp -d)"
wget -P "${TEMPD}" https://wiki.telink-semi.cn/tools_and_sdk/BLE/B85M_BLE_SDK.zip
unzip "${TEMPD}/B85M_BLE_SDK.zip" -d "${TEMPD}"

# Updating the SDK with patch "patch_0001_20231201"
mv "${TEMPD}/telink_b85m_ble_sdk_V4.0.1.3_Patch/patch_0001_20231201/proj_lib/"* "${TEMPD}/b85m_ble_sdk/proj_lib"
rmdir "${TEMPD}/telink_b85m_ble_sdk_V4.0.1.3_Patch/patch_0001_20231201/proj_lib/"
mv "${TEMPD}/telink_b85m_ble_sdk_V4.0.1.3_Patch/patch_0001_20231201/sdk_version.txt" "${TEMPD}/b85m_ble_sdk/sdk_version.txt"

# Install updated SDK to the target directory
mv "${TEMPD}/b85m_ble_sdk" "${TEL_PATH}"

# Add doc (with patch "patch_0001_20231201")
mv "${TEMPD}/doc" "${TEL_PATH}"
mv "${TEMPD}/patch_note.md" "${TEL_PATH}/doc"
mv "${TEMPD}/telink_b85m_ble_sdk_V4.0.1.3_Patch/patch_0001_20231201/Patch_Guide.txt" "${TEL_PATH}/doc"

# Remove temporary directories
rm "${TEMPD}/B85M_BLE_SDK.zip" "${TEMPD}/telink_b85m_ble_sdk_V4.0.1.3_Patch/patch_note.md"
rmdir "${TEMPD}/telink_b85m_ble_sdk_V4.0.1.3_Patch/patch_0001_20231201" "${TEMPD}/telink_b85m_ble_sdk_V4.0.1.3_Patch" "${TEMPD}"

echo -e "\nSDK installation completed.\n"
exit 0
