/**
 * nRF24L01+ driver for STM32.
 *
 * This is just the SPI driver bit.
 *
 * Daniel Holth <dholth@fastmail.fm>, 2013-2014
 */

#include "nRF24L01-dwh.h"
#include "HardwareSPI.h"

#include "config/stm32plus.h"
#include "config/spi.h"

using namespace stm32plus;

namespace nrf {
typedef Spi1<> OurSPI;
OurSPI *spi;
// CE on PA3
GpioA<DefaultDigitalOutputFeature<3> > pa;

/**
 * Turn on SPI etc...
 */
void init() {
	OurSPI::Parameters senderParams;
	senderParams.spi_mode = SPI_Mode_Master;
	spi = new OurSPI(senderParams);
}

void csh() {
	spi->setNss(true);
}

void csl() {
	spi->setNss(false);
}

uint8_t read_register() {
	uint8_t status;
	uint8_t reg;
	csl();
	// read register 0
	spi->receive(&status, 1);
	// send dummy byte to shift out response
	spi->receive(&reg, 1);
	csh();
	return reg;
}
}

void HardwareSPI::init(void) {
	// RF24 intends this to initialize the GPIOs.
	nrf::init();
}

void HardwareSPI::begin(void) {
	// RF24 intends this to init the SPI bus.
	// Better reference Arduino SPI class...
}

auto HardwareSPI::transfer(uint8_t data) -> uint8_t {
	// send the byte
	uint8_t rx;
	nrf::spi->send(&data, 1, &rx);
	return rx;
}

// Chip enable. RX/TX happen when enabled.
void HardwareSPI::ce(bool high) {
	nrf::pa[3].setState(high);
}

// Chip select not / NSS. Set false to enable chip.
void HardwareSPI::csn(bool high) {
	nrf::spi->setNss(high);
}
