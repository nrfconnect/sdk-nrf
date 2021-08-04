/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <devicetree.h>
#include <drivers/gpio.h>
#include <string.h>
#include <sys/__assert.h>
#include <hal/nrf_gpio.h>

static void fem_pin_num_correction(uint8_t *p_gpio_pin, const char *gpio_lbl)
{
	(void)p_gpio_pin;

	/* The pin numbering in the FEM configuration API follows the
	 * convention:
	 *   pin numbers 0..31 correspond to the gpio0 port
	 *   pin numbers 32..63 correspond to the gpio1 port
	 *
	 * Values obtained from devicetree are here adjusted to the ranges
	 * given above.
	 */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
	if (strcmp(gpio_lbl, DT_LABEL(DT_NODELABEL(gpio0))) == 0) {
		return;
	}
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	if (strcmp(gpio_lbl, DT_LABEL(DT_NODELABEL(gpio1))) == 0) {
		*p_gpio_pin += 32;
		return;
	}
#endif

	__ASSERT(false, "Unknown GPIO port DT label");
}

static int fem_nrf21540_gpio_pins_forward(void)
{
#if !DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_fem))
#error Node with label 'nrf_radio_fem' not found in the devicetree.
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), tx_en_gpios)
	uint8_t tx_en_pin = DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), tx_en_gpios);

	fem_pin_num_correction(&tx_en_pin, DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem), tx_en_gpios));
	nrf_gpio_pin_mcu_select(tx_en_pin, NRF_GPIO_PIN_MCUSEL_NETWORK);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), rx_en_gpios)
	uint8_t rx_en_pin = DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), rx_en_gpios);

	fem_pin_num_correction(&rx_en_pin, DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem), rx_en_gpios));
	nrf_gpio_pin_mcu_select(rx_en_pin, NRF_GPIO_PIN_MCUSEL_NETWORK);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), pdn_gpios)
	uint8_t pdn_pin = DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), pdn_gpios);

	fem_pin_num_correction(&pdn_pin, DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem), pdn_gpios));
	nrf_gpio_pin_mcu_select(pdn_pin, NRF_GPIO_PIN_MCUSEL_NETWORK);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), mode_gpios)
	uint8_t mode_pin = DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), mode_gpios);

	fem_pin_num_correction(&mode_pin, DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem), mode_gpios));
	nrf_gpio_pin_mcu_select(mode_pin, NRF_GPIO_PIN_MCUSEL_NETWORK);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	uint8_t ant_sel_pin = DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios);

	fem_pin_num_correction(&ant_sel_pin,
			       DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios));
	nrf_gpio_pin_mcu_select(ant_sel_pin, NRF_GPIO_PIN_MCUSEL_NETWORK);
#endif

#if DT_NODE_HAS_STATUS(MPSL_FEM_SPI_IF, okay)
	uint8_t cs_pin = DT_SPI_DEV_CS_GPIOS_PIN(MPSL_FEM_SPI_IF);
	uint8_t sck_pin = DT_PROP(DT_BUS(DT_NODELABEL(nrf_radio_fem_spi)), sck_pin);
	uint8_t miso_pin = DT_PROP(DT_BUS(DT_NODELABEL(nrf_radio_fem_spi)), miso_pin);
	uint8_t mosi_pin = DT_PROP(DT_BUS(DT_NODELABEL(nrf_radio_fem_spi)), mosi_pin);

	fem_pin_num_correction(&cs_pin, DT_SPI_DEV_CS_GPIOS_LABEL(MPSL_FEM_SPI_IF));
	nrf_gpio_pin_mcu_select(cs_pin, NRF_GPIO_PIN_MCUSEL_NETWORK);
	nrf_gpio_pin_mcu_select(sck_pin, NRF_GPIO_PIN_MCUSEL_NETWORK);
	nrf_gpio_pin_mcu_select(miso_pin, NRF_GPIO_PIN_MCUSEL_NETWORK);
	nrf_gpio_pin_mcu_select(mosi_pin, NRF_GPIO_PIN_MCUSEL_NETWORK);
#endif

	return 0;
}

static int fem_simple_gpio_pins_forward(void)
{
#if !DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_fem))
#error Node with label 'nrf_radio_fem' not found in the devicetree.
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ctx_gpios)
	uint8_t ctx_pin = DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), ctx_gpios);

	fem_pin_num_correction(&ctx_pin, DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem), ctx_gpios));
	nrf_gpio_pin_mcu_select(ctx_pin, NRF_GPIO_PIN_MCUSEL_NETWORK);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), crx_gpios)
	uint8_t crx_pin = DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), crx_gpios);

	fem_pin_num_correction(&crx_pin, DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem), crx_gpios));
	nrf_gpio_pin_mcu_select(crx_pin, NRF_GPIO_PIN_MCUSEL_NETWORK);
#endif

	return 0;
}

static int mpsl_fem_host_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int err = 0;

	if (IS_ENABLED(CONFIG_MPSL_FEM_NRF21540_GPIO)) {
		err = fem_nrf21540_gpio_pins_forward();
	} else if (IS_ENABLED(CONFIG_MPSL_FEM_SIMPLE_GPIO)) {
		err = fem_simple_gpio_pins_forward();
	} else {
		__ASSERT(false, "Incomplete CONFIG_MPSL_FEM configuration. "
				 "No supported FEM type found.");
	}

	return err;
}

SYS_INIT(mpsl_fem_host_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
