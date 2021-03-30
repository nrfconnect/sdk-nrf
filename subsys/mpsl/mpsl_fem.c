/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <devicetree.h>
#include <drivers/gpio.h>
#include <string.h>
#include <sys/__assert.h>
#include <mpsl_fem_config_nrf21540_gpio.h>
#include <mpsl_fem_config_simple_gpio.h>
#include <nrfx_gpiote.h>
#if IS_ENABLED(CONFIG_HAS_HW_NRF_PPI)
#include <nrfx_ppi.h>
#elif IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC)
#include <nrfx_dppi.h>
#endif

#define MPSL_FEM_GPIO_POLARITY_GET(dt_property) \
	((GPIO_ACTIVE_LOW & \
	  DT_GPIO_FLAGS(DT_NODELABEL(nrf_radio_fem), dt_property)) \
	 ? false : true)

#define MPSL_FEM_SPI_IF DT_PHANDLE(DT_NODELABEL(nrf_radio_fem), spi_if)

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
static int inactive_pin_configure(uint8_t pin, const char *gpio_lbl,
				  gpio_flags_t flags)
{
	const struct device *port;

	if (gpio_lbl != NULL) {
		port = device_get_binding(gpio_lbl);
	} else {
		if (pin < 32) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
			port = device_get_binding(
				DT_LABEL(DT_NODELABEL(gpio0)));
#else
			__ASSERT(false, "Unknown GPIO port DT label");
#endif
		} else {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
			pin -= 32;
			port = device_get_binding(
				DT_LABEL(DT_NODELABEL(gpio1)));
#else
			__ASSERT(false, "Unknown GPIO port DT label");
#endif
		}
	}

	if (!port) {
		return -EIO;
	} else {
		return gpio_pin_configure(
			port, pin, GPIO_OUTPUT_INACTIVE | flags);
	}
}
#endif

static int ppi_channel_alloc(uint8_t *ppi_channels, size_t size)
{
	nrfx_err_t err = NRFX_ERROR_NOT_SUPPORTED;

	for (int i = 0; i < size; i++) {
		IF_ENABLED(CONFIG_HAS_HW_NRF_PPI,
			(err = nrfx_ppi_channel_alloc(&ppi_channels[i]);));
		IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
			(err = nrfx_dppi_channel_alloc(&ppi_channels[i]);));
		if (err != NRFX_SUCCESS) {
			return -ENOMEM;
		}
	}

	return 0;
}

#if IS_ENABLED(CONFIG_MPSL_FEM_NRF21540_GPIO)
static int fem_nrf21540_gpio_configure(void)
{
#if !DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_fem))
#error Node with label 'nrf_radio_fem' not found in the devicetree.
#endif
	int err;

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), tx_en_gpios)
	uint8_t txen_gpiote_channel;

	if (nrfx_gpiote_channel_alloc(&txen_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), rx_en_gpios)
	uint8_t rxen_gpiote_channel;

	if (nrfx_gpiote_channel_alloc(&rxen_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), pdn_gpios)
	uint8_t pdn_gpiote_channel;

	if (nrfx_gpiote_channel_alloc(&pdn_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
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
			.gpiote_ch_id = txen_gpiote_channel
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
			.gpiote_ch_id = rxen_gpiote_channel
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
			.gpiote_ch_id = pdn_gpiote_channel
#else
			MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
		}
	};

	err = ppi_channel_alloc(cfg.ppi_channels, ARRAY_SIZE(cfg.ppi_channels));
	if (err) {
		return err;
	}

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

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), mode_gpios)
	err = inactive_pin_configure(
		DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), mode_gpios),
		DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem), mode_gpios),
		DT_GPIO_FLAGS(DT_NODELABEL(nrf_radio_fem), mode_gpios));

	if (err) {
		return err;
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	err = inactive_pin_configure(
		DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios),
		DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios),
		DT_GPIO_FLAGS(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios));

	if (err) {
		return err;
	}
#endif

#if DT_NODE_HAS_STATUS(MPSL_FEM_SPI_IF, okay)
	err = inactive_pin_configure(
		DT_PROP(DT_BUS(DT_NODELABEL(nrf_radio_fem_spi)), sck_pin),
		NULL,
		0);

	if (err) {
		return err;
	}

	err = inactive_pin_configure(
		DT_PROP(DT_BUS(DT_NODELABEL(nrf_radio_fem_spi)), miso_pin),
		NULL,
		0);

	if (err) {
		return err;
	}

	err = inactive_pin_configure(
		DT_PROP(DT_BUS(DT_NODELABEL(nrf_radio_fem_spi)), mosi_pin),
		NULL,
		0);

	if (err) {
		return err;
	}

	err = inactive_pin_configure(
		DT_SPI_DEV_CS_GPIOS_PIN(MPSL_FEM_SPI_IF),
		DT_SPI_DEV_CS_GPIOS_LABEL(MPSL_FEM_SPI_IF),
		DT_SPI_DEV_CS_GPIOS_FLAGS(MPSL_FEM_SPI_IF));

	if (err) {
		return err;
	}
#endif
	(void)err;

	return mpsl_fem_nrf21540_gpio_interface_config_set(&cfg);
}
#endif

#if IS_ENABLED(CONFIG_MPSL_FEM_SIMPLE_GPIO)
static int fem_simple_gpio_configure(void)
{
#if !DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_fem))
#error Node with label 'nrf_radio_fem' not found in the devicetree.
#endif
	int err;

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ctx_gpios)
	uint8_t ctx_gpiote_channel;

	if (nrfx_gpiote_channel_alloc(&ctx_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), crx_gpios)
	uint8_t crx_gpiote_channel;

	if (nrfx_gpiote_channel_alloc(&crx_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
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
			.gpiote_ch_id = ctx_gpiote_channel
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
			.gpiote_ch_id = crx_gpiote_channel
#else
			MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
		}
	};

	err = ppi_channel_alloc(cfg.ppi_channels, ARRAY_SIZE(cfg.ppi_channels));
	if (err) {
		return err;
	}

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
