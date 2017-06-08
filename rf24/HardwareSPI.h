/**
 * HardwareSPI emulation for STM32
 * (What RF24 needs for standard SPI, plus the ce pin)
 */

#pragma once

#include <stdint.h>

class HardwareSPI {
public:
    void init(void);
    void begin(void);
    void csn(bool high);
    void ce(bool high);
    auto transfer(uint8_t data) -> uint8_t;
};
