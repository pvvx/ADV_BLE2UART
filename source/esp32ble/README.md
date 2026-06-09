# ESP32-C3 / ESP32-C6 Build

Use WSL (Ubuntu) to build. Run all commands from this directory.

## Quick start

```bash
./install.sh                                            # default: esp32c3, no LED
./install.sh --device esp32c6 --rgb 15 --adv rgb        # for C6 with RGB on GPIO15

./build.sh                                              # build (default target)
./build.sh --device esp32c6 --rgb 15 --adv rgb          # build for C6 with RGB
```

Both `install.sh` and `build.sh` accept the same `--device`, `--led`, `--rgb`,
and `--adv` options. Run `./install.sh --help` or `./build.sh --help` for details.

| Flag | Description |
|------|-------------|
| `--device <target>` | Target chip (esp32c3, esp32c6, esp32h2, ...) |
| `--led <gpio>` | Regular LED on this GPIO |
| `--rgb <gpio>` | RGB LED (SK6812) on this GPIO |
| `--adv <mode>` | Adv blink mode: `led`, `rgb`, `none` |

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
