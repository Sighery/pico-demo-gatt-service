# pico-demo-gatt-service
Pico GATT service example

# Summary
This is a Bluetooth GATT service example for Raspberry Pi Pico W and other Bluetooth enabled boards supported by the [Pico SDK].
It was based on [btstack GATT counter] example.

# Rational
While [Pico examples] lets you build the btstack examples it does so by avoiding duplication of CMake code at the expense of simplicity.
Here we provide a minimalist btstack example which can readily be used as a template for your own project.

[Pico SDK]: https://github.com/raspberrypi/pico-sdk
[btstack GATT counter]: https://github.com/bluekitchen/btstack/blob/master/example/gatt_counter.c
[Pico examples]: https://github.com/raspberrypi/pico-examples
