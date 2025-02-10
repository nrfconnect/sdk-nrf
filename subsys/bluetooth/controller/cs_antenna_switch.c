/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include "cs_antenna_switch.h"

#include <hal/nrf_gpio.h>

#define DEFAULT_CS_ANTENNA_GPIO_PORT NRF_P1
#define DEFAULT_CS_ANTENNA_BASE_PIN (11)
#define DEFAULT_CS_ANTENNA_PIN_MASK (0xF << DEFAULT_CS_ANTENNA_BASE_PIN)

/* Antenna control below is implemented as described in the CS documentation.
 * https://docs.nordicsemi.com/bundle/ncs-latest/page/nrfxlib/softdevice_controller/doc/channel_sounding.html#multiple_antennas_support
 * The board has four antenna ports (ANT1-ANT4) that can be individually
 * enabled by setting the connected gpio pin high, while setting the gpio
 * pins for the other antenna ports low.
 *
 * Map of antenna ports to gpio pins for nrf54xDK:
 * ANT1 <----> P1.11
 * ANT2 <----> P1.12
 * ANT3 <----> P1.13
 * ANT4 <----> P1.14
 */
void cs_antenna_switch_func(uint8_t antenna_number)
{
	uint32_t out = nrf_gpio_port_out_read(DEFAULT_CS_ANTENNA_GPIO_PORT);

	out &= ~DEFAULT_CS_ANTENNA_PIN_MASK;
	out |= 1 << (DEFAULT_CS_ANTENNA_BASE_PIN + antenna_number);
	nrf_gpio_port_out_write(DEFAULT_CS_ANTENNA_GPIO_PORT, out);
}

void cs_antenna_switch_enable(void)
{
	nrf_gpio_port_dir_output_set(DEFAULT_CS_ANTENNA_GPIO_PORT, DEFAULT_CS_ANTENNA_BASE_PIN);
}
