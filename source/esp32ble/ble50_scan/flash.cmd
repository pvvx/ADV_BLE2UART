@echo off
REM esptool v5.3.0 — https://github.com/espressif/esptool/releases
REM Usage: flash.cmd COM3 [chip]
REM        chip defaults to esp32c3; e.g. flash.cmd COM7 esp32c6
if "%2"=="" (
    set CHIP=esp32c3
) else (
    set CHIP=%2
)
esptool.exe --chip %CHIP% --port %1 -b 460800 write-flash --flash-mode dio --flash-size 2MB --flash-freq 80m 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/ble50_scan.bin