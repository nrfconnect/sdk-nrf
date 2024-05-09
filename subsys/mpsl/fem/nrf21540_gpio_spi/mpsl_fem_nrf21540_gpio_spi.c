/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if !defined(CONFIG_MPSL_FEM_PIN_FORWARDER)

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <soc_nrf_common.h>

#include <mpsl_fem_utils.h>

#include <mpsl_fem_config_nrf21540_gpio_spi.h>
#if IS_ENABLED(CONFIG_MPSL_FEM_POWER_MODEL)
#include "mpsl_fem_power_model_interface.h"
#endif
#include <nrfx_gpiote.h>

#if !defined(CONFIG_PINCTRL)
#error CONFIG_PINCTRL is required for nRF21540 GPIO SPI driver
#endif

#define MPSL_FEM_SPI_IF    DT_PHANDLE(DT_NODELABEL(nrf_radio_fem), spi_if)
#define MPSL_FEM_SPI_BUS   DT_BUS(MPSL_FEM_SPI_IF)
#define MPSL_FEM_SPI_REG   ((NRF_SPIM_Type *) DT_REG_ADDR(MPSL_FEM_SPI_BUS))

#define MPSL_FEM_SPI_GPIO_PORT(pin)     DT_GPIO_CTLR(MPSL_FEM_SPI_BUS, pin)
#define MPSL_FEM_SPI_GPIO_PORT_REG(pin) ((NRF_GPIO_Type *)DT_REG_ADDR(MPSL_FEM_SPI_GPIO_PORT(pin)))
#define MPSL_FEM_SPI_GPIO_PORT_NO(pin)  DT_PROP(MPSL_FEM_SPI_GPIO_PORT(pin), port)
#define MPSL_FEM_SPI_GPIO_PIN_NO(pin)   DT_GPIO_PIN(MPSL_FEM_SPI_BUS, pin)

#define FEM_SPI_PIN_NUM(node_id, prop, idx) \
	NRF_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),

#define FEM_SPI_PIN_FUNC(node_id, prop, idx) \
	NRF_GET_FUN(DT_PROP_BY_IDX(node_id, prop, idx)),

static uint32_t fem_nrf21540_spi_configure(mpsl_fem_nrf21540_gpio_spi_interface_config_t *cfg)
{
#if DT_NODE_HAS_PROP(MPSL_FEM_SPI_BUS, cs_gpios)
	uint8_t cs_gpiote_channel;
	const nrfx_gpiote_t cs_gpiote = NRFX_GPIOTE_INSTANCE(
		NRF_DT_GPIOTE_INST(MPSL_FEM_SPI_BUS, cs_gpios));

	if (nrfx_gpiote_channel_alloc(&cs_gpiote, &cs_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif

	cfg->spi_config = (mpsl_fem_spi_config_t) {
		.p_spim = MPSL_FEM_SPI_REG,
		.cs_pin_config = {
#if DT_NODE_HAS_PROP(MPSL_FEM_SPI_BUS, cs_gpios)
			.gpio_pin = {
				.p_port        = MPSL_FEM_SPI_GPIO_PORT_REG(cs_gpios),
				.port_no       = MPSL_FEM_SPI_GPIO_PORT_NO(cs_gpios),
				.port_pin      = MPSL_FEM_SPI_GPIO_PIN_NO(cs_gpios),
			},
			.enable        = true,
			.active_high   = true,
			.gpiote_ch_id  = cs_gpiote_channel
#else
			MPSL_FEM_DISABLED_GPIOTE_PIN_CONFIG_INIT
#endif
		}
	};

	static const uint8_t fem_spi_pin_nums[] = {
		DT_FOREACH_CHILD_VARGS(
			DT_PINCTRL_BY_NAME(DT_BUS(MPSL_FEM_SPI_IF), default, 0),
			DT_FOREACH_PROP_ELEM, psels, FEM_SPI_PIN_NUM
		)
	};

	static const uint8_t fem_spi_pin_funcs[] = {
		DT_FOREACH_CHILD_VARGS(
			DT_PINCTRL_BY_NAME(DT_BUS(MPSL_FEM_SPI_IF), default, 0),
			DT_FOREACH_PROP_ELEM, psels, FEM_SPI_PIN_FUNC
		)
	};

	for (size_t i = 0U; i < ARRAY_SIZE(fem_spi_pin_nums); i++) {
		switch (fem_spi_pin_funcs[i]) {
		case NRF_FUN_SPIM_SCK:
			mpsl_fem_extended_pin_to_mpsl_fem_pin(fem_spi_pin_nums[i],
							      &cfg->spi_config.sck_pin);
			break;
		case NRF_FUN_SPIM_MOSI:
			mpsl_fem_extended_pin_to_mpsl_fem_pin(fem_spi_pin_nums[i],
							      &cfg->spi_config.mosi_pin);
			break;
		case NRF_FUN_SPIM_MISO:
			mpsl_fem_extended_pin_to_mpsl_fem_pin(fem_spi_pin_nums[i],
							      &cfg->spi_config.miso_pin);
			break;
		default:
			break;
		}
	}

	return 0;
}

static int fem_nrf21540_gpio_spi_configure(void)
{
	int err;

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), tx_en_gpios)
	uint8_t txen_gpiote_channel;
	const nrfx_gpiote_t txen_gpiote = NRFX_GPIOTE_INSTANCE(
		NRF_DT_GPIOTE_INST(DT_NODELABEL(nrf_radio_fem), tx_en_gpios));

	if (nrfx_gpiote_channel_alloc(&txen_gpiote, &txen_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), rx_en_gpios)
	uint8_t rxen_gpiote_channel;
	const nrfx_gpiote_t rxen_gpiote = NRFX_GPIOTE_INSTANCE(
		NRF_DT_GPIOTE_INST(DT_NODELABEL(nrf_radio_fem), rx_en_gpios));

	if (nrfx_gpiote_channel_alloc(&rxen_gpiote, &rxen_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), pdn_gpios)
	uint8_t pdn_gpiote_channel;
	const nrfx_gpiote_t pdn_gpiote = NRFX_GPIOTE_INSTANCE(
		NRF_DT_GPIOTE_INST(DT_NODELABEL(nrf_radio_fem), pdn_gpios));

	if (nrfx_gpiote_channel_alloc(&pdn_gpiote, &pdn_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif

	mpsl_fem_nrf21540_gpio_spi_interface_config_t cfg = {
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
			.pa_gain_db =
				CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB,
			.pa_gains_db = {
				[0] = CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTA,
				[1] = CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTB,
			},
			.lna_gain_db     =
				CONFIG_MPSL_FEM_NRF21540_RX_GAIN_DB
		},
		.pa_pin_config = {
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), tx_en_gpios)
			.gpio_pin      = {
				.p_port   = MPSL_FEM_GPIO_PORT_REG(tx_en_gpios),
				.port_no  = MPSL_FEM_GPIO_PORT_NO(tx_en_gpios),
				.port_pin = MPSL_FEM_GPIO_PIN_NO(tx_en_gpios),
			},
			.enable        = true,
			.active_high   = MPSL_FEM_GPIO_POLARITY_GET(tx_en_gpios),
			.gpiote_ch_id  = txen_gpiote_channel
#else
			MPSL_FEM_DISABLED_GPIOTE_PIN_CONFIG_INIT
#endif
		},
		.lna_pin_config = {
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), rx_en_gpios)
			.gpio_pin      = {
				.p_port   = MPSL_FEM_GPIO_PORT_REG(rx_en_gpios),
				.port_no  = MPSL_FEM_GPIO_PORT_NO(rx_en_gpios),
				.port_pin = MPSL_FEM_GPIO_PIN_NO(rx_en_gpios),
			},
			.enable        = true,
			.active_high   = MPSL_FEM_GPIO_POLARITY_GET(rx_en_gpios),
			.gpiote_ch_id  = rxen_gpiote_channel
#else
			MPSL_FEM_DISABLED_GPIOTE_PIN_CONFIG_INIT
#endif
		},
		.pdn_pin_config = {
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), pdn_gpios)
			.gpio_pin      = {
				.p_port   = MPSL_FEM_GPIO_PORT_REG(pdn_gpios),
				.port_no  = MPSL_FEM_GPIO_PORT_NO(pdn_gpios),
				.port_pin = MPSL_FEM_GPIO_PIN_NO(pdn_gpios),
			},
			.enable        = true,
			.active_high   = MPSL_FEM_GPIO_POLARITY_GET(pdn_gpios),
			.gpiote_ch_id  = pdn_gpiote_channel
#else
			MPSL_FEM_DISABLED_GPIOTE_PIN_CONFIG_INIT
#endif
		},
		.mode_pin_config = {
			MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
		},
		.pa_gain_runtime_control =
			IS_ENABLED(CONFIG_MPSL_FEM_NRF21540_RUNTIME_PA_GAIN_CONTROL),
	};

	err = fem_nrf21540_spi_configure(&cfg);
	if (err) {
		return err;
	}

	IF_ENABLED(CONFIG_HAS_HW_NRF_PPI,
		   (err = mpsl_fem_utils_ppi_channel_alloc(cfg.ppi_channels,
							   ARRAY_SIZE(cfg.ppi_channels));));
	IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
		   (err = mpsl_fem_utils_ppi_channel_alloc(cfg.dppi_channels,
							   ARRAY_SIZE(cfg.dppi_channels));));
	if (err) {
		return err;
	}

	IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
		   (err = mpsl_fem_utils_egu_instance_alloc(&cfg.egu_instance_no);));
	if (err) {
		return err;
	}

	IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
		   (err = mpsl_fem_utils_egu_channel_alloc(cfg.egu_channels,
							   ARRAY_SIZE(cfg.egu_channels),
							   cfg.egu_instance_no);))
	if (err) {
		return err;
	}

	BUILD_ASSERT(
	  (CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB == CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTA) ||
	  (CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB == CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTB));

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), mode_gpios)
	/* Configure mode_gpios pin as inactive. */
	err = gpio_pin_configure(
		DEVICE_DT_GET(MPSL_FEM_GPIO_PORT(mode_gpios)),
		MPSL_FEM_GPIO_PIN_NO(mode_gpios),
		GPIO_OUTPUT_INACTIVE | DT_GPIO_FLAGS(DT_NODELABEL(nrf_radio_fem), mode_gpios));

	if (err) {
		return err;
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	/* Configure ant_sel_gpios pin as inactive. */
	err = gpio_pin_configure(
		DEVICE_DT_GET(MPSL_FEM_GPIO_PORT(ant_sel_gpios)),
		MPSL_FEM_GPIO_PIN_NO(ant_sel_gpios),
		GPIO_OUTPUT_INACTIVE | DT_GPIO_FLAGS(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios));

	if (err) {
		return err;
	}
#endif

#if IS_ENABLED(CONFIG_MPSL_FEM_POWER_MODEL)
	const mpsl_fem_power_model_t *power_model = mpsl_fem_power_model_to_use_get();

	mpsl_fem_power_model_set(power_model);

	if (err) {
		return err;
	}
#endif /* IS_ENABLED(CONFIG_MPSL_FEM_POWER_MODEL) */

	err = mpsl_fem_nrf21540_gpio_spi_interface_config_set(&cfg);
	return err;
}

static int mpsl_fem_init(void)
{

	mpsl_fem_device_config_254_apply_set(IS_ENABLED(CONFIG_MPSL_FEM_DEVICE_CONFIG_254));

	return fem_nrf21540_gpio_spi_configure();
}

SYS_INIT(mpsl_fem_init, POST_KERNEL, CONFIG_MPSL_FEM_INIT_PRIORITY);

#endif /* !defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */
