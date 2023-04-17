/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <zephyr/sys/__assert.h>

#if !defined(CONFIG_MPSL_FEM_PIN_FORWARDER)
#include <mpsl_fem_config_nrf21540_gpio.h>
#if IS_ENABLED(CONFIG_MPSL_FEM_POWER_MODEL)
#include "mpsl_fem_power_model_interface.h"
#endif
#include <nrfx_gpiote.h>
#include <mpsl_fem_utils.h>
#else /* !defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */
#include <soc_secure.h>
#if IS_ENABLED(CONFIG_PINCTRL)
#include <pinctrl_soc.h>
#endif
#endif /* !defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */

#if !defined(CONFIG_MPSL_FEM_PIN_FORWARDER)
static int fem_nrf21540_gpio_configure(void)
{
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
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), mode_gpios) && \
	IS_ENABLED(CONFIG_MPSL_FEM_NRF21540_RUNTIME_PA_GAIN_CONTROL)
			.gpio_pin      = {
				.p_port   = MPSL_FEM_GPIO_PORT_REG(mode_gpios),
				.port_no  = MPSL_FEM_GPIO_PORT_NO(mode_gpios),
				.port_pin = MPSL_FEM_GPIO_PIN_NO(mode_gpios),
			},
			.enable        = true,
			.active_high   = MPSL_FEM_GPIO_POLARITY_GET(mode_gpios),
#else
			MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
		}
	};

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
	if (!IS_ENABLED(CONFIG_MPSL_FEM_NRF21540_RUNTIME_PA_GAIN_CONTROL)) {
		gpio_flags_t mode_pin_flags = GPIO_OUTPUT_ACTIVE;

		if (CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB == CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTA) {
			mode_pin_flags = GPIO_OUTPUT_INACTIVE;
		}

		mode_pin_flags |= DT_GPIO_FLAGS(DT_NODELABEL(nrf_radio_fem), mode_gpios);

		err = gpio_pin_configure(
			DEVICE_DT_GET(MPSL_FEM_GPIO_PORT(mode_gpios)),
			MPSL_FEM_GPIO_PIN_NO(mode_gpios),
			mode_pin_flags);

		if (err) {
			return err;
		}
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

	(void)err;

	return mpsl_fem_nrf21540_gpio_interface_config_set(&cfg);
}

static int mpsl_fem_init(void)
{

#if IS_ENABLED(CONFIG_MPSL_FEM_POWER_MODEL)
	int err;
	const mpsl_fem_power_model_t *power_model = mpsl_fem_power_model_to_use_get();

	err = mpsl_fem_power_model_set(power_model);

	if (err) {
		return err;
	}
#endif /* IS_ENABLED(CONFIG_MPSL_FEM_POWER_MODEL) */

	mpsl_fem_device_config_254_apply_set(IS_ENABLED(CONFIG_MPSL_FEM_DEVICE_CONFIG_254));

	return fem_nrf21540_gpio_configure();
}

SYS_INIT(mpsl_fem_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#else /* !defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */

static int mpsl_fem_host_init(void)
{

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), tx_en_gpios)
	uint8_t tx_en_pin = NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(nrf_radio_fem), tx_en_gpios);

	soc_secure_gpio_pin_mcu_select(tx_en_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), rx_en_gpios)
	uint8_t rx_en_pin = NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(nrf_radio_fem), rx_en_gpios);

	soc_secure_gpio_pin_mcu_select(rx_en_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), pdn_gpios)
	uint8_t pdn_pin = NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(nrf_radio_fem), pdn_gpios);

	soc_secure_gpio_pin_mcu_select(pdn_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), mode_gpios)
	uint8_t mode_pin = NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(nrf_radio_fem), mode_gpios);

	soc_secure_gpio_pin_mcu_select(mode_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)
	uint8_t ant_sel_pin = NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios);

	soc_secure_gpio_pin_mcu_select(ant_sel_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif

	return 0;

}

SYS_INIT(mpsl_fem_host_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* !defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */
