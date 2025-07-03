/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hal_led.h"
#include "pico/cyw43_arch.h"

// Defined by btstack, though not stricly needed I guess as long as you are not using it
void hal_led_toggle(void) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN));
}

void led_off() {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}

void led_on() {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}

bool led_status() {
    return cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN);
}
