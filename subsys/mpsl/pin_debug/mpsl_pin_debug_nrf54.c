/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <mpsl_dppi_protocol_api.h>
#include <nrfx_gpiote.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_dppi.h>
#include <hal/nrf_radio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

const nrfx_gpiote_t gpiote = NRFX_GPIOTE_INSTANCE(20);
const nrfx_dppi_t dppi_radio_domain = NRFX_DPPI_INSTANCE(10);
const nrfx_dppi_t dppi_gpio_domain = NRFX_DPPI_INSTANCE(20);
LOG_MODULE_REGISTER(mpsl_radio_pin_debug, CONFIG_MPSL_LOG_LEVEL);

static int m_ppi_config(void)
{
	uint8_t gppi_chan_radio_ready;
	uint8_t gppi_chan_radio_disabled;
	uint8_t gppi_chan_radio_address;
	uint8_t gppi_chan_radio_end;

	uint8_t dppi_chan_gpio_ready;
	uint8_t dppi_chan_gpio_disabled;
	uint8_t dppi_chan_gpio_address;
	uint8_t dppi_chan_gpio_end;

	if (nrfx_gppi_channel_alloc(&gppi_chan_radio_ready) !=
	    NRFX_SUCCESS) {
		LOG_ERR("Failed allocating gppi_chan_radio_ready");
		return -ENOMEM;
	}

	if (nrfx_gppi_channel_alloc(&gppi_chan_radio_disabled) !=
	    NRFX_SUCCESS) {
		LOG_ERR("Failed allocating gppi_chan_radio_disabled");
		return -ENOMEM;
	}

	if (nrfx_gppi_channel_alloc(&gppi_chan_radio_address) !=
	    NRFX_SUCCESS) {
		LOG_ERR("Failed allocating gppi_chan_radio_address");
		return -ENOMEM;
	}

	if (nrfx_gppi_channel_alloc(&gppi_chan_radio_end) !=
	    NRFX_SUCCESS) {
		LOG_ERR("Failed allocating gppi_chan_radio_end");
		return -ENOMEM;
	}

	if (nrfx_dppi_channel_alloc(&dppi_gpio_domain, &dppi_chan_gpio_ready) !=
	    NRFX_SUCCESS) {
		LOG_ERR("Failed allocating dppi_chan_gpio_ready");
		return -ENOMEM;
	}

	if (nrfx_dppi_channel_alloc(&dppi_gpio_domain, &dppi_chan_gpio_disabled) !=
	    NRFX_SUCCESS) {
		LOG_ERR("Failed allocating dppi_chan_gpio_disabled");
		return -ENOMEM;
	}

	if (nrfx_dppi_channel_alloc(&dppi_gpio_domain, &dppi_chan_gpio_address) !=
	    NRFX_SUCCESS) {
		LOG_ERR("Failed allocating dppi_chan_gpio_address");
		return -ENOMEM;
	}

	if (nrfx_dppi_channel_alloc(&dppi_gpio_domain, &dppi_chan_gpio_end) !=
	    NRFX_SUCCESS) {
		LOG_ERR("Failed allocating dppi_chan_gpio_end");
		return -ENOMEM;
	}

	if (nrfx_gppi_edge_connection_setup(gppi_chan_radio_ready,
					    &dppi_radio_domain,
					    MPSL_DPPI_RADIO_PUBLISH_READY_CHANNEL_IDX,
					    &dppi_gpio_domain,
					    dppi_chan_gpio_ready) != NRFX_SUCCESS) {
		LOG_ERR("Failed edge setup chan ready");
		return -ENOMEM;
	}

	if (nrfx_gppi_edge_connection_setup(gppi_chan_radio_disabled,
					    &dppi_radio_domain,
					    MPSL_DPPI_RADIO_PUBLISH_DISABLED_CH_IDX,
					    &dppi_gpio_domain,
					    dppi_chan_gpio_disabled) != NRFX_SUCCESS) {
		LOG_ERR("Failed edge setup chan disabled");
		return -ENOMEM;
	}

	if (nrfx_gppi_edge_connection_setup(gppi_chan_radio_address,
					    &dppi_radio_domain,
					    MPSL_DPPI_RADIO_PUBLISH_ADDRESS_CHANNEL_IDX,
					    &dppi_gpio_domain,
					    dppi_chan_gpio_address) != NRFX_SUCCESS) {
		LOG_ERR("Failed edge setup chan address");
		return -ENOMEM;
	}

	/* Setup a PPI bridge between the radio domain and the domain of the GPIO pin. */
	if (nrfx_gppi_edge_connection_setup(gppi_chan_radio_address,
					    &dppi_radio_domain,
					    MPSL_DPPI_RADIO_PUBLISH_END_CHANNEL_IDX,
					    &dppi_gpio_domain,
					    dppi_chan_gpio_end) != NRFX_SUCCESS) {
		LOG_ERR("Failed edge setup chan end");
		return -ENOMEM;
	}

	nrf_gpiote_subscribe_set(
		gpiote.p_reg,
		nrfx_gpiote_set_task_address_get(
			&gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN),
		dppi_chan_gpio_ready);

	nrf_gpiote_subscribe_set(
		gpiote.p_reg,
		nrfx_gpiote_clr_task_address_get(
			&gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN),
		dppi_chan_gpio_disabled);

	nrf_gpiote_subscribe_set(
		gpiote.p_reg,
		nrfx_gpiote_set_task_address_get(&gpiote,
						 CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN),
		dppi_chan_gpio_address);

	nrf_gpiote_subscribe_set(
		gpiote.p_reg,
		nrfx_gpiote_clr_task_address_get(&gpiote,
						 CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN),
		dppi_chan_gpio_end);

	nrfx_gppi_channels_enable(NRFX_BIT(gppi_chan_radio_ready) |
				  NRFX_BIT(gppi_chan_radio_disabled) |
				  NRFX_BIT(gppi_chan_radio_address) |
				  NRFX_BIT(gppi_chan_radio_end));

	if (nrfx_dppi_channel_enable(&dppi_gpio_domain, dppi_chan_gpio_ready) != NRFX_SUCCESS) {
		LOG_ERR("Failed chan enable gpio_ready");
		return -ENOMEM;
	}

	if (nrfx_dppi_channel_enable(&dppi_gpio_domain, dppi_chan_gpio_disabled) != NRFX_SUCCESS) {
		LOG_ERR("Failed chan enable gpio_disabled");
		return -ENOMEM;
	}

	if (nrfx_dppi_channel_enable(&dppi_gpio_domain, dppi_chan_gpio_address) != NRFX_SUCCESS) {
		LOG_ERR("Failed chan enable gpio_address");
		return -ENOMEM;
	}

	if (nrfx_dppi_channel_enable(&dppi_gpio_domain, dppi_chan_gpio_end) != NRFX_SUCCESS) {
		LOG_ERR("Failed chan enable gpio_end");
		return -ENOMEM;
	}

	return 0;
}

static int mpsl_radio_pin_debug_init(void)
{
	uint8_t radio_ready_radio_disabled_gpiote_channel;
	uint8_t radio_address_radio_end_gpiote_channel;

	const nrfx_gpiote_output_config_t gpiote_output_cfg = NRFX_GPIOTE_DEFAULT_OUTPUT_CONFIG;

	if (nrfx_gpiote_channel_alloc(&gpiote, &radio_ready_radio_disabled_gpiote_channel) !=
	    NRFX_SUCCESS) {
		LOG_ERR("Failed allocating GPIOTE chan");
		return -ENOMEM;
	}

	if (nrfx_gpiote_channel_alloc(&gpiote, &radio_address_radio_end_gpiote_channel) !=
	    NRFX_SUCCESS) {
		LOG_ERR("Failed allocating GPIOTE chan");
		return -ENOMEM;
	}

	const nrfx_gpiote_task_config_t task_cfg_ready_disabled = {
		.task_ch = radio_ready_radio_disabled_gpiote_channel,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
	};

	if (nrfx_gpiote_output_configure(&gpiote,
					 CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN,
					 &gpiote_output_cfg,
					 &task_cfg_ready_disabled)
	    != NRFX_SUCCESS) {
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
