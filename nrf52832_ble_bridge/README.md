# nRF52832 BLE EQ Bridge

This subproject is a Nordic nRF Connect SDK application that bridges BLE Nordic UART Service (NUS) traffic to the STM32 EQ control UART.

## Function

- Phone app sends ASCII EQ commands over BLE NUS.
- nRF52832 forwards the bytes to STM32 over UART.
- STM32 replies with `OK`, `ERR`, or `EQ STATUS`, and the bridge forwards that data back to BLE.

## Default Wiring

- nRF52832 `P0.06` TX -> STM32 `PA3` RX
- nRF52832 `P0.08` RX <- STM32 `PA2` TX
- GND -> GND

UART pins are configured in `app.overlay`.

## Build

This project was validated with NCS `v3.2.2` on this machine.

### VS Code nRF Connect Extension

1. Open the `nrf52832_ble_bridge` folder as an application.
2. Add a build configuration for board `nrf52dk/nrf52832`.
3. Click Build.

### Command Line

If your NCS install is under `C:\ncs\v3.2.2` and the toolchain ID is `c717907b94`, this command works:

```powershell
$env:ZEPHYR_BASE='C:\ncs\v3.2.2\zephyr'
$env:PATH='C:\ncs\toolchains\c717907b94\opt\bin\Scripts;C:\ncs\toolchains\c717907b94\opt\bin;C:\ncs\toolchains\c717907b94\usr\bin;'+$env:PATH
& 'C:\ncs\toolchains\c717907b94\opt\bin\Scripts\west.exe' build -b nrf52dk/nrf52832 -s 'd:\desktop\Mood_1.0\nrf52832_ble_bridge' -d 'd:\desktop\Mood_1.0\nrf52832_ble_bridge\build' --pristine always
```

The generated image is:

- `build/merged.hex`

## Flash

Using west:

```powershell
& 'C:\ncs\toolchains\c717907b94\opt\bin\Scripts\west.exe' flash -d 'd:\desktop\Mood_1.0\nrf52832_ble_bridge\build'
```

If you use the VS Code extension, flash from the build configuration directly.

## BLE Test

Advertised device name:

- `Mood_EQ_Bridge`

Use Nordic Toolbox or nRF Connect mobile app, connect to NUS, and send lines such as:

```text
EQ ON
EQ STATUS
EQ PRESET 2
EQ BAND 1 80
EQ BAND 5 60
VOL 28
```

Each command must end with a newline.

## Notes

- Console/log output uses RTT, not UART.
- UART baud rate is `115200`.
- The STM32 side currently applies EQ to 16-bit WAV playback buffers.