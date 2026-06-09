@echo off
REM esptool v5.3.0 — https://github.com/espressif/esptool/releases
REM Usage: flash.cmd COM3 [chip] [freq]
REM        chip defaults to esp32c3; e.g. flash.cmd COM7 esp32c6
REM        freq defaults to 80m (48m for esp32h2/h4/h21); e.g. flash.cmd COM3 esp32h2 48m
REM
REM Valid chip targets: esp8266, esp32, esp32s2, esp32s3, esp32c3, esp32c2,
REM                     esp32c6, esp32c61, esp32c5, esp32e22, esp32h2,
REM                     esp32h21, esp32p4, esp32h4, esp32s31

setlocal enabledelayedexpansion

set "VALID_TARGETS= esp8266 esp32 esp32s2 esp32s3 esp32c3 esp32c2 esp32c6 esp32c61 esp32c5 esp32e22 esp32h2 esp32h21 esp32p4 esp32h4 esp32s31 "

if "%2"=="" (
    set CHIP=esp32c3
) else (
    set CHIP=%2
)

REM Validate the chip target
echo !VALID_TARGETS! | findstr /i /c:" %CHIP% " >nul
if errorlevel 1 (
    echo Error: invalid or unknown chip target '%CHIP%'.
    echo Valid targets: esp8266 esp32 esp32s2 esp32s3 esp32c3 esp32c2 esp32c6
    echo                esp32c61 esp32c5 esp32e22 esp32h2 esp32h21 esp32p4 esp32h4 esp32s31
    exit /b 1
)

REM Determine flash frequency: default 80m, but ESP32-Hx series max is 48m.
if "%3"=="" (
    set FREQ=80m
    set CHIP_PREFIX=!CHIP:~0,6!
    if "!CHIP_PREFIX!"=="esp32h" set FREQ=48m
) else (
    set FREQ=%3
)

esptool.exe --chip %CHIP% --port %1 -b 460800 write-flash --flash-mode dio --flash-size 2MB --flash-freq %FREQ% 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/ble50_scan.bin