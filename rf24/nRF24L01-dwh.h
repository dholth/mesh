/**
 * nRF24L01+ library for the stm32.
 */

#pragma once

#include <stdint.h>

#include "config/stm32plus.h"
#include "config/gpio.h"

namespace nrf {
    void init();
    uint8_t read_register();

    // Using GPIOA.

    enum {
        NSS_PIN = GPIO_Pin_4,
        CE_PIN  = GPIO_Pin_1,
        IRQ_PIN = GPIO_Pin_0
    };
}
