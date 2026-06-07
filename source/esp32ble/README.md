# ESP32-C3 / ESP32-C6 Build

Use WSL (Ubuntu) to build. Run all commands from this directory.

## Quick start

```bash
./install.sh                                            # default: esp32c3 + supermini-c3
./install.sh --device esp32c6 --config esp32-c6-gpio15  # for ESP32-C6 Super Mini (RGB GPIO15)

./build.sh                                              # build (default target)
./build.sh --device esp32c6 --config esp32-c6-gpio15    # build for C6
```

Both `install.sh` and `build.sh` accept the same `--device` and `--config` options.
Run `./install.sh --help` or `./build.sh --help` to see all available board configurations.

## Fast rebuild (after first build)

```bash
make -C ble50_scan/build -j"$(nproc)"
```

## Flash (from Windows CMD)

```cmd
pushd Z:\home\myuser\ADV_BLE2UART\source\esp32-c3\ble50_scan
flash.cmd COM3              # ESP32-C3
flash.cmd COM7 esp32c6      # ESP32-C6
popd
```

## Documentation

See [`ble50_scan/README.md`](ble50_scan/README.md) for detailed documentation:
firmware characteristics, board configurations, host tools, and protocol details.
