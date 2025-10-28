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
#include <hal/nrf_radio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

const nrfx_gpiote_t gpiote = NRFX_GPIOTE_INSTANCE(20);
LOG_MODULE_REGISTER(mpsl_radio_pin_debug, CONFIG_MPSL_LOG_LEVEL);

static int m_ppi_config(void)
{
	uint32_t rad_domain = nrfx_gppi_domain_id_get(NRF_DPPIC10);
	uint32_t dst_domain = nrfx_gppi_domain_id_get(gpiote.p_reg);
	nrfx_gppi_resource_t rad_resource;
	nrfx_gppi_handle_t handle;
	uint32_t tep[4];
	int err;
	static const uint32_t pub_ch[] = {
		MPSL_DPPI_RADIO_PUBLISH_READY_CHANNEL_IDX,
		MPSL_DPPI_RADIO_PUBLISH_DISABLED_CH_IDX,
		MPSL_DPPI_RADIO_PUBLISH_ADDRESS_CHANNEL_IDX,
		MPSL_DPPI_RADIO_PUBLISH_END_CHANNEL_IDX
	};

	tep[0] = nrfx_gpiote_set_task_address_get(&gpiote,
			CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN);
	tep[1] = nrfx_gpiote_clr_task_address_get(&gpiote,
			CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN);
	tep[2] = nrfx_gpiote_set_task_address_get(&gpiote,
			CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN);
	tep[3] = nrfx_gpiote_clr_task_address_get(&gpiote,
			CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN);
	rad_resource.rad_domain = nrfx_gppi_domain_id_get(NRF_DPPIC10);

	for (size_t i = 0; i < ARRAY_SIZE(pub_ch); i++) {
		rad_resource.channel = pub_ch[i];
		err = nrfx_gppi_ext_conn_alloc(rad_domain, dst_domain, &handle, &rad_resource);
		if (err < 0) {
			return err;
		}
		nrfx_gppi_ep_attach(handle, tep[i]);
		/* Channel in radio domain is not enabled by this function. */
		nrfx_gppi_conn_enable(handle);
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
