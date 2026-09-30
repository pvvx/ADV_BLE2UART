#!/usr/bin/env bash

# Installation script for Telink BLE SDK V4.0.2.5 from GitHub.
# The SDK repository stores the actual SDK under tc_ble_sdk/; this script
# installs that content directly into ./SDK without keeping a nested .git repo.

TEL_PATH=./SDK
SDK_TAG=V4.0.2.5
SDK_REPO=https://github.com/telink-semi/tc_ble_sdk.git

# The native proj_lib/liblt_8258.a of the tag is installed as it is: it is not
# buggy and no library swap is needed any more.
#
# The "firmware freezes when scanning on Coded PHY" symptom this project chased
# for a long time was entirely application-side.  Three defects had to be fixed
# in source/ble.c, all re-measured on hardware with the native V4.0.2.5 library
# on the same TB-03F-KIT board:
#
#   1. incomplete Link-Layer initialisation.  The configuration that works is
#      the one of the stock vendor/acl_central_demo: extended scanning +
#      blc_ll_init2MPhyCodedPhy_feature() + legacy initiating + ACL connection
#      and central role + the ACL FIFOs + blc_hci_registerControllerDataHandler()
#      + blc_gap_init() and the master MTU buffer.  Without it the extended scan
#      never receives anything and the firmware stops.
#   2. blc_ll_initPeriodicAdvertisingSynchronization_module() enabled although
#      this firmware never creates a periodic sync.  The working reference does
#      not enable it, and with it the firmware stops as soon as a Coded packet
#      arrives.
#   3. scan window equal to the scan interval, i.e. 100% duty cycle on each PHY
#      (200% with 1M + Coded enabled together).  The interval is now derived
#      from the window (source/ble.c, SCAN_INTERVAL_FROM_WINDOW).
#
# The old V4.0.2.1 library swap (USE_LEGACY_LIBRARY) has been removed: it was
# only a workaround for the symptoms above.

# sha256 of proj_lib/liblt_8258.a of this tag (sanity check)
LIB_SHA_V4025="32e898ce5fff52b40477cbde31976563d39e2597046cd29cfcd7cbdb4f25f3c6"

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

got="$(sha256sum "${TEL_PATH}/proj_lib/liblt_8258.a" | cut -d' ' -f1)"
echo "proj_lib/liblt_8258.a sha256: ${got}"
if [ "${got}" != "${LIB_SHA_V4025}" ]; then
	echo "WARNING: unexpected liblt_8258.a hash (expected ${LIB_SHA_V4025} for ${SDK_TAG})" >&2
fi

echo
echo "SDK ${SDK_TAG} installation completed with its native library."
