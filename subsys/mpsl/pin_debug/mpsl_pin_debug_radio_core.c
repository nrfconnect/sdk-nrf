/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>
#include <helpers/nrfx_gppi.h>
#include <hal/nrf_radio.h>

#if defined(DPPI_PRESENT)
#include <mpsl_dppi_protocol_api.h>
#endif

#define GPIOTE_NODE DT_NODELABEL(gpiote0)

LOG_MODULE_REGISTER(mpsl_radio_pin_debug, CONFIG_MPSL_LOG_LEVEL);

static int m_ppi_config(void)
{
	nrfx_gpiote_t *gpiote = &GPIOTE_NRFX_INST_BY_NODE(GPIOTE_NODE);
	nrfx_gppi_handle_t handle[4];
	uint32_t tep[4];

	tep[0] = nrfx_gpiote_set_task_address_get(gpiote,
			CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN);
	tep[1] = nrfx_gpiote_clr_task_address_get(gpiote,
			CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN);
	tep[2] = nrfx_gpiote_set_task_address_get(gpiote,
			CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN);
	tep[3] = nrfx_gpiote_clr_task_address_get(gpiote,
			CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN);
#if defined(DPPI_PRESENT)
	handle[0] = MPSL_DPPI_RADIO_PUBLISH_READY_CHANNEL_IDX;
	handle[1] = MPSL_DPPI_RADIO_PUBLISH_DISABLED_CH_IDX;
	handle[2] = MPSL_DPPI_RADIO_PUBLISH_ADDRESS_CHANNEL_IDX;
	handle[3] = MPSL_DPPI_RADIO_PUBLISH_END_CHANNEL_IDX;
#else
	uint32_t eep[4];

	eep[0] = nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_READY);
	eep[1] = nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);
	eep[2] = nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS);
	eep[3] = nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_END);
#endif
	for (size_t i = 0; ARRAY_SIZE(tep); i++) {
#if defined(DPPI_PRESENT)
		nrfx_gppi_ep_attach(tep[i], handle[i]);
#else
		int err = nrfx_gppi_conn_alloc(eep[i], tep[i], &handle[i]);

		if (err < 0) {
			return err;
		}
		nrfx_gppi_conn_enable(handle[i]);
#endif
	}

	return 0;
}

static int mpsl_radio_pin_debug_init(void)
{
	nrfx_gpiote_t *gpiote = &GPIOTE_NRFX_INST_BY_NODE(GPIOTE_NODE);
	uint8_t radio_ready_radio_disabled_gpiote_channel;
	uint8_t radio_address_radio_end_gpiote_channel;

	const nrfx_gpiote_output_config_t gpiote_output_cfg = NRFX_GPIOTE_DEFAULT_OUTPUT_CONFIG;

	if (nrfx_gpiote_channel_alloc(gpiote, &radio_ready_radio_disabled_gpiote_channel) != 0) {
		LOG_ERR("Failed allocating GPIOTE chan");
		return -ENOMEM;
	}

	if (nrfx_gpiote_channel_alloc(gpiote, &radio_address_radio_end_gpiote_channel) != 0) {
		LOG_ERR("Failed allocating GPIOTE chan");
		return -ENOMEM;
	}

	const nrfx_gpiote_task_config_t task_cfg_ready_disabled = {
		.task_ch = radio_ready_radio_disabled_gpiote_channel,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
	};

	if (nrfx_gpiote_output_configure(
		    gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN, &gpiote_output_cfg,
		    &task_cfg_ready_disabled) != 0) {
		LOG_ERR("Failed configuring GPIOTE chan");
		return -ENOMEM;
	}

	const nrfx_gpiote_task_config_t task_cfg_address_end = {
		.task_ch = radio_address_radio_end_gpiote_channel,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
	};

	if (nrfx_gpiote_output_configure(gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN,
					 &gpiote_output_cfg,
					 &task_cfg_address_end) != 0) {
		LOG_ERR("Failed configuring GPIOTE chan");
		return -ENOMEM;
	}

	if (m_ppi_config() != 0) {
		return -ENOMEM;
	}

	nrfx_gpiote_out_task_enable(gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN);
	nrfx_gpiote_out_task_enable(gpiote, CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN);

	return 0;
}

SYS_INIT(mpsl_radio_pin_debug_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
