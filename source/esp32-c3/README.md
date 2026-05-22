# ESP32-C3 Build

Use WSL (Ubuntu) to build.

Run all commands from this directory.

## Install

Install prerequisites, ESP-IDF, and the local ESP32-C3 toolchain:

```bash
./install.sh
```

Clean the local toolchain and generated build files:

```bash
./install.sh clean
```

## Clean Build

Run a full clean firmware build:

```bash
./build.sh
```

## Fast Rebuild

After the first successful build, run an incremental rebuild:

```bash
make -C ble50_scan/build -j"$(nproc)"
```

## Generated files

```
ble50_scan/build/ble50_scan.bin
ble50_scan/build/bootloader/bootloader.bin
ble50_scan/build/partition_table/partition-table.bin
```

## Flash

From WSL:

```
wslpath -w ble50_scan
```

From Windows:

```cmd
pushd <output of wslpath -w ble50_scan>

: Flash
flash.cmd COM3

: Run
python3 adv2uart.py COM3

popd
```
