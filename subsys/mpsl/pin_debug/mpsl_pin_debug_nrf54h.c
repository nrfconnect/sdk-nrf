/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>
#include <mpsl_dppi_protocol_api.h>
#include <nrfx_gpiote.h>
#include <nrfx_dppi.h>
#include <hal/nrf_radio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <uicr/uicr.h>

static nrfx_gpiote_t gpiote = NRFX_GPIOTE_INSTANCE(0);
LOG_MODULE_REGISTER(mpsl_radio_pin_debug, CONFIG_MPSL_LOG_LEVEL);

static int m_ppi_config(void)
{
	nrf_gpiote_subscribe_set(
		gpiote.p_reg,
		nrfx_gpiote_set_task_address_get(
			&gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN),
		MPSL_DPPI_RADIO_PUBLISH_READY_CHANNEL_IDX);

	nrf_gpiote_subscribe_set(
		gpiote.p_reg,
		nrfx_gpiote_clr_task_address_get(
			&gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN),
		MPSL_DPPI_RADIO_PUBLISH_DISABLED_CH_IDX);

	nrf_gpiote_subscribe_set(gpiote.p_reg,
				 nrfx_gpiote_set_task_address_get(
					 &gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN),
				 MPSL_DPPI_RADIO_PUBLISH_ADDRESS_CHANNEL_IDX);

	nrf_gpiote_subscribe_set(gpiote.p_reg,
				 nrfx_gpiote_clr_task_address_get(
					 &gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN),
				 MPSL_DPPI_RADIO_PUBLISH_END_CHANNEL_IDX);

	return 0;
}

/* SPU131: P1.4 permissions */
UICR_SPU_FEATURE_GPIO_PIN_SET(0x5f920000UL, 1, 4, true, NRF_OWNER_RADIOCORE);
/* SPU131: P1.5 permissions */
UICR_SPU_FEATURE_GPIO_PIN_SET(0x5f920000UL, 1, 5, true, NRF_OWNER_RADIOCORE);
/* gpio1 - P1.4 CTRLSEL = 4 */
UICR_GPIO_PIN_CNF_CTRLSEL_SET(0x5F938200UL, 4, 6);
/* gpio1 - P1.5 CTRLSEL = 4 */
UICR_GPIO_PIN_CNF_CTRLSEL_SET(0x5F938200UL, 5, 6);

static int mpsl_radio_pin_debug_init(void)
{
	if (IS_ENABLED(CONFIG_PM_DEVICE)) {
		const struct device *gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));

		int ret = pm_device_runtime_get(gpio1);

		if (ret) {
			LOG_ERR("Failed to get runtime for GPIO: %d", ret);
			return ret;
		}
	}

	/* GPIOTE channel allocation is not available for GPIOTE0.
	 * We use channel 0 and 1 to match NRF_P1 pin 4 and 5.
	 */
	const uint8_t radio_ready_radio_disabled_gpiote_channel = 0;
	const uint8_t radio_address_radio_end_gpiote_channel = 1;

	const nrfx_gpiote_output_config_t gpiote_output_cfg = NRFX_GPIOTE_DEFAULT_OUTPUT_CONFIG;

	const nrfx_gpiote_task_config_t task_cfg_ready_disabled = {
		.task_ch = radio_ready_radio_disabled_gpiote_channel,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
	};

	if (nrfx_gpiote_output_configure(
		    &gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN, &gpiote_output_cfg,
		    &task_cfg_ready_disabled) != NRFX_SUCCESS) {
		LOG_ERR("Failed configuring GPIOTE chan");
		return -ENOMEM;
	}

	const nrfx_gpiote_task_config_t task_cfg_address_end = {
		.task_ch = radio_address_radio_end_gpiote_channel,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
	};

	if (nrfx_gpiote_output_configure(&gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN,
					 &gpiote_output_cfg,
					 &task_cfg_address_end) != NRFX_SUCCESS) {
		LOG_ERR("Failed configuring GPIOTE chan");
		return -ENOMEM;
	}

	if (m_ppi_config() != 0) {
		return -ENOMEM;
	}

	nrfx_gpiote_out_task_enable(&gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN);
	nrfx_gpiote_out_task_enable(&gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN);

	return 0;
}

SYS_INIT(mpsl_radio_pin_debug_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
