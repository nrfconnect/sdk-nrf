/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(CONFIG_MPSL_FEM_SIMPLE_GPIO)

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
#if IS_ENABLED(CONFIG_HAS_HW_NRF_PPI)
#include <nrfx_ppi.h>
#elif IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC)
#include <nrfx_dppi.h>
#endif
#else /* !defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */
#include <mpsl_fem_utils.h>
#include <soc_secure.h>
#if IS_ENABLED(CONFIG_PINCTRL)
#include <pinctrl_soc.h>
#endif
#endif /* !defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */

#if !defined(CONFIG_MPSL_FEM_PIN_FORWARDER)

#define MPSL_FEM_GPIO_POLARITY_GET(dt_property) \
	((GPIO_ACTIVE_LOW & \
	  DT_GPIO_FLAGS(DT_NODELABEL(nrf_radio_fem), dt_property)) \
	 ? false : true)

#define MPSL_FEM_GPIO_PORT(pin)     DT_GPIO_CTLR(DT_NODELABEL(nrf_radio_fem), pin)
#define MPSL_FEM_GPIO_PORT_REG(pin) ((NRF_GPIO_Type *)DT_REG_ADDR(MPSL_FEM_GPIO_PORT(pin)))
#define MPSL_FEM_GPIO_PORT_NO(pin)  DT_PROP(MPSL_FEM_GPIO_PORT(pin), port)
#define MPSL_FEM_GPIO_PIN_NO(pin)   DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), pin)

#define MPSL_FEM_GPIO_INVALID_PIN        0xFFU
#define MPSL_FEM_GPIOTE_INVALID_CHANNEL  0xFFU
#define MPSL_FEM_DISABLED_GPIOTE_PIN_CONFIG_INIT	\
	.gpio_pin      = {				\
		.port_pin = MPSL_FEM_GPIO_INVALID_PIN,	\
	},						\
	.enable        = false,				\
	.active_high   = true,				\
	.gpiote_ch_id  = MPSL_FEM_GPIOTE_INVALID_CHANNEL

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

#if defined(CONFIG_HAS_HW_NRF_DPPIC)
static int egu_instance_alloc(uint8_t *p_egu_instance_no)
{
	/* Always return EGU0. */
	*p_egu_instance_no = 0;

	return 0;
}

static int egu_channel_alloc(uint8_t *egu_channels, size_t size, uint8_t egu_instance_no)
{
	(void)egu_instance_no;

	/* The 802.15.4 radio driver is the only user of EGU peripheral on nRF5340 network core
	 * and it uses channels: 0, 1, 2, 3, 4, 15. Therefore starting from channel 5, a consecutive
	 * block of at most 10 channels can be allocated.
	 */
	if (size > 10U) {
		return -ENOMEM;
	}

	uint8_t starting_channel = 5U;

	for (int i = 0; i < size; i++) {
		egu_channels[i] = starting_channel + i;
	}

	return 0;
}
#endif

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
		   (err = ppi_channel_alloc(cfg.ppi_channels, ARRAY_SIZE(cfg.ppi_channels));));
	IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
		   (err = ppi_channel_alloc(cfg.dppi_channels, ARRAY_SIZE(cfg.dppi_channels));));
	if (err) {
		return err;
	}

	IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
		   (err = egu_instance_alloc(&cfg.egu_instance_no);));
	if (err) {
		return err;
	}

	IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
		   (err = egu_channel_alloc(cfg.egu_channels,
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

#endif /* defined(CONFIG_MPSL_FEM_SIMPLE_GPIO) */
