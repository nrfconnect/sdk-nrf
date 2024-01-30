/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrfx_gpiote.h>
#include <helpers/nrfx_gppi.h>
#include <hal/nrf_radio.h>

#ifdef PPI_PRESENT
#include <nrfx_ppi.h>
#elif defined(DPPI_PRESENT)
#include <nrfx_dppi.h>
#include <mpsl_dppi_protocol_api.h>
#include <hal/nrf_egu.h>
#endif

LOG_MODULE_REGISTER(mpsl_radio_pin_debug, CONFIG_MPSL_LOG_LEVEL);

static int m_ppi_config(void)
{
#if defined(PPI_PRESENT)
	uint8_t ppi_chan_radio_ready;
	uint8_t ppi_chan_radio_address;
	uint8_t ppi_chan_radio_end;
	uint8_t ppi_chan_radio_disabled;

	if (nrfx_ppi_channel_alloc(&ppi_chan_radio_ready) != NRFX_SUCCESS) {
		LOG_ERR("Failed allocating PPI chan");
		return -ENOMEM;
	}

	if (nrfx_ppi_channel_alloc(&ppi_chan_radio_address) != NRFX_SUCCESS) {
		LOG_ERR("Failed allocating PPI chan");
		return -ENOMEM;
	}

	if (nrfx_ppi_channel_alloc(&ppi_chan_radio_end) != NRFX_SUCCESS) {
		LOG_ERR("Failed allocating PPI chan");
		return -ENOMEM;
	}

	if (nrfx_ppi_channel_alloc(&ppi_chan_radio_disabled) != NRFX_SUCCESS) {
		LOG_ERR("Failed allocating PPI chan");
		return -ENOMEM;
	}

	nrfx_gppi_channel_endpoints_setup(
		ppi_chan_radio_ready, nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_READY),
		nrfx_gpiote_out_task_address_get(
			CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN));

	nrfx_gppi_channel_endpoints_setup(
		ppi_chan_radio_disabled,
		nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_DISABLED),
		nrfx_gpiote_out_task_address_get(
			CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN));

	nrfx_gppi_channel_endpoints_setup(
		ppi_chan_radio_address,
		nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS),
		nrfx_gpiote_out_task_address_get(CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN));

	nrfx_gppi_channel_endpoints_setup(
		ppi_chan_radio_end, nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_END),
		nrfx_gpiote_out_task_address_get(CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN));

	if (nrfx_ppi_channel_enable(ppi_chan_radio_ready) != NRFX_SUCCESS) {
		LOG_ERR("Failed enabling channel");
		return -ENOMEM;
	}
	if (nrfx_ppi_channel_enable(ppi_chan_radio_address) != NRFX_SUCCESS) {
		LOG_ERR("Failed enabling channel");
		return -ENOMEM;
	}
	if (nrfx_ppi_channel_enable(ppi_chan_radio_end) != NRFX_SUCCESS) {
		LOG_ERR("Failed enabling channel");
		return -ENOMEM;
	}
	if (nrfx_ppi_channel_enable(ppi_chan_radio_disabled) != NRFX_SUCCESS) {
		LOG_ERR("Failed enabling channel");
		return -ENOMEM;
	}

#elif defined(DPPI_PRESENT)
	/* Radio events are published on predefined channels.
	 */
	uint8_t ppi_chan_radio_ready = MPSL_DPPI_RADIO_PUBLISH_READY_CHANNEL_IDX;
	uint8_t ppi_chan_radio_address = MPSL_DPPI_RADIO_PUBLISH_ADDRESS_CHANNEL_IDX;
	uint8_t ppi_chan_radio_end = MPSL_DPPI_RADIO_PUBLISH_END_CHANNEL_IDX;
	uint8_t ppi_chan_radio_disabled = MPSL_DPPI_RADIO_PUBLISH_DISABLED_CH_IDX;

	/* Because the GPIOTE needs to subscribe to only two DPPI channels,
	 * we use an EGU to route events together.
	 */
	uint8_t dppi_chan_ready_disabled;
	uint8_t dppi_chan_address_end;

	if (nrfx_dppi_channel_alloc(&dppi_chan_ready_disabled) != NRFX_SUCCESS) {
		LOG_ERR("Failed allocating DPPI chan");
		return -ENOMEM;
	}

	if (nrfx_dppi_channel_alloc(&dppi_chan_address_end) != NRFX_SUCCESS) {
		LOG_ERR("Failed allocating DPPI chan");
		return -ENOMEM;
	}
	nrf_egu_subscribe_set(NRF_EGU0, NRF_EGU_TASK_TRIGGER0, ppi_chan_radio_ready);
	nrf_egu_subscribe_set(NRF_EGU0, NRF_EGU_TASK_TRIGGER1, ppi_chan_radio_disabled);
	nrf_egu_subscribe_set(NRF_EGU0, NRF_EGU_TASK_TRIGGER2, ppi_chan_radio_address);
	nrf_egu_subscribe_set(NRF_EGU0, NRF_EGU_TASK_TRIGGER3, ppi_chan_radio_end);

	nrf_egu_publish_set(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED0, dppi_chan_ready_disabled);
	nrf_egu_publish_set(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED1, dppi_chan_ready_disabled);
	nrf_egu_publish_set(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED2, dppi_chan_address_end);
	nrf_egu_publish_set(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED3, dppi_chan_address_end);

	nrfx_gppi_task_endpoint_setup(dppi_chan_ready_disabled,
				      nrfx_gpiote_out_task_address_get(
					      CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN));

	nrfx_gppi_task_endpoint_setup(
		dppi_chan_address_end,
		nrfx_gpiote_out_task_address_get(CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN));

	if (nrfx_dppi_channel_enable(dppi_chan_ready_disabled) != NRFX_SUCCESS) {
		LOG_ERR("Failed enabling channel");
		return -ENOMEM;
	}

	if (nrfx_dppi_channel_enable(dppi_chan_address_end) != NRFX_SUCCESS) {
		LOG_ERR("Failed enabling channel");
		return -ENOMEM;
	}
#else
#error "Expect either PPI or DPPI to be present."
#endif

	return 0;
}

static int mpsl_radio_pin_debug_init(void)
{
	uint8_t radio_ready_radio_disabled_gpiote_channel;
	uint8_t radio_address_radio_end_gpiote_channel;

	const nrfx_gpiote_output_config_t gpiote_output_cfg = NRFX_GPIOTE_DEFAULT_OUTPUT_CONFIG;

	if (nrfx_gpiote_channel_alloc(&radio_ready_radio_disabled_gpiote_channel) != NRFX_SUCCESS) {
		LOG_ERR("Failed allocating GPIOTE chan");
		return -ENOMEM;
	}

	if (nrfx_gpiote_channel_alloc(&radio_address_radio_end_gpiote_channel) != NRFX_SUCCESS) {
		LOG_ERR("Failed allocating GPIOTE chan");
		return -ENOMEM;
	}

	const nrfx_gpiote_task_config_t task_cfg_ready_disabled = {
		.task_ch = radio_ready_radio_disabled_gpiote_channel,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
	};

	if (nrfx_gpiote_output_configure(CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN,
					 &gpiote_output_cfg,
					 &task_cfg_ready_disabled) != NRFX_SUCCESS) {
		LOG_ERR("Failed configuring GPIOTE chan");
		return -ENOMEM;
	}

	const nrfx_gpiote_task_config_t task_cfg_address_end = {
		.task_ch = radio_address_radio_end_gpiote_channel,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
	};

	if (nrfx_gpiote_output_configure(CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN,
					 &gpiote_output_cfg,
					 &task_cfg_address_end) != NRFX_SUCCESS) {
		LOG_ERR("Failed configuring GPIOTE chan");
		return -ENOMEM;
	}

	if (m_ppi_config() != 0) {
		return -ENOMEM;
	}

	nrfx_gpiote_out_task_enable(CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN);
	nrfx_gpiote_out_task_enable(CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN);

	return 0;
}

SYS_INIT(mpsl_radio_pin_debug_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
