# pico-demo-gatt-service
Pico GATT service example

# Summary
This is a Bluetooth GATT service example for Raspberry Pi Pico W and other Bluetooth enabled boards supported by the [Pico SDK].
It was based on [btstack GATT counter] example.

# Rational
While [Pico examples] lets you build the btstack examples it does so by avoiding duplication of CMake code at the expense of simplicity.
Here we provide a simpler btstack example which can readily be used as a template for your own project.

# Prerequisites
- Visual Studio Code
- [Pico VS Code Extension]
- Raspberry Pi Pico W plugged over USB
- Connected [Raspberry Pi Debug Probe] to automatically flash the firmware

# Build
- Import the project directory in VS Code making sure you enable the CMake-Tools extension integration option
- Check that the selected board matches the board you want to test against
- Hit <kbd>Ctrl + Shift + B</kbd> to build and flash the project to your board
- Without Debug Probe you will need to copy the UF2 file from `\build\pico_demo_gatt_serive.uf2` to your board after connecting it while holding down BOOTSEL button

[Pico SDK]: https://github.com/raspberrypi/pico-sdk
[btstack GATT counter]: https://github.com/bluekitchen/btstack/blob/master/example/gatt_counter.c
[Pico examples]: https://github.com/raspberrypi/pico-examples
[Raspberry Pi Debug Probe]: https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html
[Pico VS Code Extension]: https://github.com/raspberrypi/pico-vscode
