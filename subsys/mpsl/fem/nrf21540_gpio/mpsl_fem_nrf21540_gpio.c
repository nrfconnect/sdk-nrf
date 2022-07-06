/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(CONFIG_MPSL_FEM_NRF21540_GPIO)

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

#define MPSL_FEM_DISABLED_GPIO_CONFIG_INIT		\
	.gpio_pin      = {				\
		.port_pin = MPSL_FEM_GPIO_INVALID_PIN,	\
	},						\
	.enable        = false,				\
	.active_high   = true,				\

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

#if IS_ENABLED(CONFIG_MPSL_FEM_NRF21540_GPIO)
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
#endif

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

	return fem_nrf21540_gpio_configure();
}

SYS_INIT(mpsl_fem_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#else /* !defined(CONFIG_MPSL_FEM_PIN_FORWARDER) */

static int mpsl_fem_host_init(const struct device *dev)
{
	ARG_UNUSED(dev);

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

#endif /* defined(CONFIG_MPSL_FEM_NRF21540_GPIO) */
