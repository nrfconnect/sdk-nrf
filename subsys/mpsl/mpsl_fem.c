/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <devicetree.h>
#include <drivers/gpio.h>
#include <string.h>
#include <sys/__assert.h>
#include <mpsl_fem_config_nrf21540_gpio.h>
#include <mpsl_fem_config_simple_gpio.h>

#define MPSL_FEM_GPIO_POLARITY_GET(dt_property) \
	((GPIO_ACTIVE_LOW & \
	  DT_GPIO_FLAGS(DT_NODELABEL(nrf_radio_fem), dt_property)) \
	 ? false : true)

#define MPSL_FEM_GPIO_INVALID_PIN        0xFFU
#define MPSL_FEM_GPIOTE_INVALID_CHANNEL  0xFFU
#define MPSL_FEM_DISABLED_GPIO_CONFIG_INIT \
	.enable       = false, \
	.active_high  = true, \
	.gpio_pin     = MPSL_FEM_GPIO_INVALID_PIN, \
	.gpiote_ch_id = MPSL_FEM_GPIOTE_INVALID_CHANNEL

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

#if IS_ENABLED(CONFIG_MPSL_FEM_NRF21540_GPIO)
static int fem_nrf21540_gpio_configure(void)
{
	/* FEM configuration requires gpiote and ppi channels.
	 * Currently there is no reliable common method to dynamically
	 * allocate such channels. FEM module needs only "some" channels
	 * to use whichever they are, but FEM needs them for exclusive use
	 * and does not enable them immediately.
	 *
	 * When common api to assign gpiote and ppi channels is available
	 * current solution based on macros coming from Kconfig should be
	 * reworked.
	 */

#if !DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_fem))
#error Node with label 'nrf_radio_fem' not found in the devicetree.
#endif

	mpsl_fem_nrf21540_gpio_interface_config_t cfg = {
		.fem_config = {
			.pa_time_gap_us  =
				DT_PROP(DT_NODELABEL(nrf_radio_fem),
					tx_en_settle_time_us),
			.lna_time_gap_us =
				DT_PROP(DT_NODELABEL(nrf_radio_fem),
					rx_en_settle_time_us),
			.pdn_settle_us   =
				DT_PROP(DT_NODELABEL(nrf_radio_fem),
					pdn_settle_time_us),
			.trx_hold_us     =
				DT_PROP(DT_NODELABEL(nrf_radio_fem),
					trx_hold_time_us),
			.pa_gain_db      =
				CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB,
			.lna_gain_db     =
				CONFIG_MPSL_FEM_NRF21540_RX_GAIN_DB
		},
		.pa_pin_config = {
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), tx_en_gpios)
			.enable       = true,
			.active_high  =
				MPSL_FEM_GPIO_POLARITY_GET(tx_en_gpios),
			.gpio_pin     =
				DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem),
					    tx_en_gpios),
			.gpiote_ch_id =
				CONFIG_MPSL_FEM_NRF21540_GPIO_GPIOTE_TX_EN
#else
			MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
		},
		.lna_pin_config = {
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), rx_en_gpios)
			.enable       = true,
			.active_high  =
				MPSL_FEM_GPIO_POLARITY_GET(rx_en_gpios),
			.gpio_pin     =
				DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem),
					    rx_en_gpios),
			.gpiote_ch_id =
				CONFIG_MPSL_FEM_NRF21540_GPIO_GPIOTE_RX_EN
#else
			MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
		},
		.pdn_pin_config = {
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), pdn_gpios)
			.enable       = true,
			.active_high  =
				MPSL_FEM_GPIO_POLARITY_GET(pdn_gpios),
			.gpio_pin     =
				DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem),
					    pdn_gpios),
			.gpiote_ch_id =
				CONFIG_MPSL_FEM_NRF21540_GPIO_GPIOTE_PDN
#else
			MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
		},
		.ppi_channels = {
			CONFIG_MPSL_FEM_NRF21540_GPIO_PPI_CHANNEL_0,
			CONFIG_MPSL_FEM_NRF21540_GPIO_PPI_CHANNEL_1,
			CONFIG_MPSL_FEM_NRF21540_GPIO_PPI_CHANNEL_2
		}
	};

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), tx_en_gpios)
	fem_pin_num_correction(&cfg.pa_pin_config.gpio_pin,
			       DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem),
					     tx_en_gpios));
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), rx_en_gpios)
	fem_pin_num_correction(&cfg.lna_pin_config.gpio_pin,
			       DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem),
					     rx_en_gpios));
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), pdn_gpios)
	fem_pin_num_correction(&cfg.pdn_pin_config.gpio_pin,
			       DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem),
					     pdn_gpios));
#endif

	return mpsl_fem_nrf21540_gpio_interface_config_set(&cfg);
}
#endif

#if IS_ENABLED(CONFIG_MPSL_FEM_SIMPLE_GPIO)
static int fem_simple_gpio_configure(void)
{
	/* FEM configuration requires gpiote and ppi channels.
	 * Currently there is no reliable common method to dynamically
	 * allocate such channels. FEM module needs only "some" channels
	 * to use whichever they are, but FEM needs them for exclusive use
	 * and does not enable them immediately.
	 *
	 * When common api to assign gpiote and ppi channels is available
	 * current solution based on macros coming from Kconfig should be
	 * reworked.
	 */

#if !DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_fem))
#error Node with label 'nrf_radio_fem' not found in the devicetree.
#endif

	mpsl_fem_simple_gpio_interface_config_t cfg = {
		.fem_config = {
			.pa_time_gap_us  =
				DT_PROP(DT_NODELABEL(nrf_radio_fem),
					ctx_settle_time_us),
			.lna_time_gap_us =
				DT_PROP(DT_NODELABEL(nrf_radio_fem),
					crx_settle_time_us),
			.pa_gain_db      =
				DT_PROP(DT_NODELABEL(nrf_radio_fem),
					tx_gain_db),
			.lna_gain_db     =
				DT_PROP(DT_NODELABEL(nrf_radio_fem),
					rx_gain_db)
		},
		.pa_pin_config = {
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ctx_gpios)
			.enable       = true,
			.active_high  =
				MPSL_FEM_GPIO_POLARITY_GET(ctx_gpios),
			.gpio_pin     =
				DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem),
					    ctx_gpios),
			.gpiote_ch_id =
				CONFIG_MPSL_FEM_SIMPLE_GPIO_GPIOTE_CTX
#else
			MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
		},
		.lna_pin_config = {
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), crx_gpios)
			.enable       = true,
			.active_high  =
				MPSL_FEM_GPIO_POLARITY_GET(crx_gpios),
			.gpio_pin     =
				DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem),
					    crx_gpios),
			.gpiote_ch_id =
				CONFIG_MPSL_FEM_SIMPLE_GPIO_GPIOTE_CRX
#else
			MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
		},
		.ppi_channels = {
			CONFIG_MPSL_FEM_SIMPLE_GPIO_PPI_CHANNEL_0,
			CONFIG_MPSL_FEM_SIMPLE_GPIO_PPI_CHANNEL_1
		}
	};

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ctx_gpios)
	fem_pin_num_correction(&cfg.pa_pin_config.gpio_pin,
			       DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem),
					     ctx_gpios));
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), crx_gpios)
	fem_pin_num_correction(&cfg.lna_pin_config.gpio_pin,
			       DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem),
					     crx_gpios));
#endif

	return mpsl_fem_simple_gpio_interface_config_set(&cfg);
}
#endif

int mpsl_fem_configure(void)
{
	int err = 0;

#if IS_ENABLED(CONFIG_MPSL_FEM_NRF21540_GPIO)
	err = fem_nrf21540_gpio_configure();
#elif IS_ENABLED(CONFIG_MPSL_FEM_SIMPLE_GPIO)
	err = fem_simple_gpio_configure();
#else
#error Incomplete CONFIG_MPSL_FEM configuration. No supported FEM type found.
#endif

	return err;
}
