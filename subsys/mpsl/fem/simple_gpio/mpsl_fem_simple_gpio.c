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
#include <mpsl_fem_config_simple_gpio.h>
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
static int fem_simple_gpio_configure(void)
{
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
			.gpio_pin      = {
				.p_port   = MPSL_FEM_GPIO_PORT_REG(ctx_gpios),
				.port_no  = MPSL_FEM_GPIO_PORT_NO(ctx_gpios),
				.port_pin = MPSL_FEM_GPIO_PIN_NO(ctx_gpios),
			},
			.enable        = true,
			.active_high   = MPSL_FEM_GPIO_POLARITY_GET(ctx_gpios),
			.gpiote_ch_id  = ctx_gpiote_channel
#else
			MPSL_FEM_DISABLED_GPIOTE_PIN_CONFIG_INIT
#endif
		},
		.lna_pin_config = {
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), crx_gpios)
			.gpio_pin      = {
				.p_port   = MPSL_FEM_GPIO_PORT_REG(crx_gpios),
				.port_no  = MPSL_FEM_GPIO_PORT_NO(crx_gpios),
				.port_pin = MPSL_FEM_GPIO_PIN_NO(crx_gpios),
			},
			.enable        = true,
			.active_high   = MPSL_FEM_GPIO_POLARITY_GET(crx_gpios),
			.gpiote_ch_id  = crx_gpiote_channel
#else
			MPSL_FEM_DISABLED_GPIOTE_PIN_CONFIG_INIT
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

	return mpsl_fem_simple_gpio_interface_config_set(&cfg);
}

static int mpsl_fem_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if IS_ENABLED(CONFIG_MPSL_FEM_POWER_MODEL)
	int err;
	const mpsl_fem_power_model_t *power_model = mpsl_fem_power_model_to_use_get();

	err = mpsl_fem_power_model_set(power_model);

	if (err) {
		return err;
	}
#endif /* IS_ENABLED(CONFIG_MPSL_FEM_POWER_MODEL) */

	mpsl_fem_device_config_254_apply_set(IS_ENABLED(CONFIG_MPSL_FEM_DEVICE_CONFIG_254));

	return fem_simple_gpio_configure();
}

SYS_INIT(mpsl_fem_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#else /* !defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */

static int mpsl_fem_host_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ctx_gpios)
	uint8_t ctx_pin = NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(nrf_radio_fem), ctx_gpios);

	soc_secure_gpio_pin_mcu_select(ctx_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), crx_gpios)
	uint8_t crx_pin = NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(nrf_radio_fem), crx_gpios);

	soc_secure_gpio_pin_mcu_select(crx_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif

	return 0;
}

SYS_INIT(mpsl_fem_host_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* !defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */
