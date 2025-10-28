/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_egu.h>
#include <hal/nrf_radio.h>
#include <hal/nrf_timer.h>
#include <hal/nrf_dppi.h>
#include <helpers/nrfx_gppi.h>

#include <zephyr/logging/log.h>

#include "esb_peripherals.h"
#include "esb_ppi_api.h"

LOG_MODULE_DECLARE(esb, CONFIG_ESB_LOG_LEVEL);

static uint8_t radio_address_timer_stop;
static uint8_t timer_compare0_radio_disable;
static uint8_t timer_compare1_radio_txen;
static uint8_t disabled_phy_end_egu;
static uint8_t egu_timer_start;
static uint8_t egu_ramp_up;
static uint8_t radio_end_timer_start;

static nrf_dppi_channel_group_t ramp_up_dppi_group;

void esb_ppi_for_txrx_set(bool rx, bool timer_start)
{
	uint32_t channels_mask;

	nrf_egu_event_clear(ESB_EGU, ESB_EGU_EVENT);
	nrf_egu_event_clear(ESB_EGU, ESB_EGU_DPPI_EVENT);

	nrf_egu_publish_set(ESB_EGU, ESB_EGU_EVENT, egu_timer_start);
	nrf_egu_publish_set(ESB_EGU, ESB_EGU_DPPI_EVENT, egu_ramp_up);

	nrf_dppi_channels_include_in_group(ESB_DPPIC, BIT(egu_ramp_up), ramp_up_dppi_group);

	nrf_egu_subscribe_set(ESB_EGU, ESB_EGU_DPPI_TASK, egu_timer_start);
	nrf_radio_subscribe_set(NRF_RADIO, rx ? NRF_RADIO_TASK_RXEN : NRF_RADIO_TASK_TXEN,
				egu_ramp_up);
	nrf_dppi_subscribe_set(ESB_DPPIC,
				nrf_dppi_group_disable_task_get((uint8_t)ramp_up_dppi_group),
				egu_ramp_up);

	nrf_egu_subscribe_set(ESB_EGU, ESB_EGU_TASK, disabled_phy_end_egu);

	if (timer_start) {
		nrf_timer_subscribe_set(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_TASK_START,
					egu_timer_start);
	}

	channels_mask = (BIT(egu_timer_start) |
			 BIT(egu_ramp_up));

	nrf_dppi_channels_enable(ESB_DPPIC, channels_mask);
}

void esb_ppi_for_txrx_clear(bool rx, bool timer_start)
{
	uint32_t channels_mask;

	channels_mask = (BIT(egu_timer_start) |
			 BIT(egu_ramp_up));

	nrf_dppi_channels_disable(ESB_DPPIC, channels_mask);

	nrf_egu_publish_clear(ESB_EGU, ESB_EGU_EVENT);
	nrf_egu_publish_clear(ESB_EGU, ESB_EGU_DPPI_EVENT);

	nrf_egu_subscribe_clear(ESB_EGU, ESB_EGU_DPPI_TASK);
	nrf_radio_subscribe_clear(NRF_RADIO, rx ? NRF_RADIO_TASK_RXEN : NRF_RADIO_TASK_TXEN);
	nrf_dppi_subscribe_clear(ESB_DPPIC,
				 nrf_dppi_group_disable_task_get((uint8_t)ramp_up_dppi_group));
	nrf_egu_subscribe_clear(ESB_EGU, ESB_EGU_TASK);

	nrf_dppi_channels_remove_from_group(ESB_DPPIC, BIT(egu_ramp_up), ramp_up_dppi_group);

	if (timer_start) {
		nrf_timer_subscribe_clear(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_TASK_START);
	}
}

void esb_ppi_for_fem_set(void)
{
	nrf_egu_publish_set(ESB_EGU, ESB_EGU_EVENT, egu_timer_start);
	nrf_timer_subscribe_set(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_TASK_START,
				egu_timer_start);

	nrf_dppi_channels_enable(ESB_DPPIC, BIT(egu_timer_start));
}

void esb_ppi_for_fem_clear(void)
{
	nrf_dppi_channels_disable(ESB_DPPIC, BIT(egu_timer_start));

	nrf_egu_publish_clear(ESB_EGU, ESB_EGU_EVENT);
	nrf_timer_subscribe_clear(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_TASK_START);
}

void esb_ppi_for_retransmission_set(void)
{
	nrf_egu_event_clear(ESB_EGU, ESB_EGU_EVENT);

	nrf_timer_publish_set(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_EVENT_COMPARE1,
			      timer_compare1_radio_txen);

	nrf_radio_subscribe_set(NRF_RADIO, NRF_RADIO_TASK_TXEN, timer_compare1_radio_txen);
	nrf_egu_subscribe_set(ESB_EGU, ESB_EGU_TASK, disabled_phy_end_egu);

	if (IS_ENABLED(CONFIG_ESB_FAST_SWITCHING)) {
		nrf_radio_subscribe_set(NRF_RADIO, NRF_RADIO_TASK_RXEN, disabled_phy_end_egu);
	}

	nrf_dppi_channels_enable(ESB_DPPIC, BIT(timer_compare1_radio_txen));
}

void esb_ppi_for_retransmission_clear(void)
{
	nrf_dppi_channels_disable(ESB_DPPIC, BIT(timer_compare1_radio_txen));

	nrf_timer_publish_clear(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_EVENT_COMPARE1);

	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_TXEN);
	nrf_egu_subscribe_clear(ESB_EGU, ESB_EGU_TASK);

	if (IS_ENABLED(CONFIG_ESB_FAST_SWITCHING)) {
		nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_RXEN);
	}
}

void esb_ppi_for_wait_for_ack_set(void)
{
	uint32_t channels_mask;

	nrf_radio_publish_set(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS, radio_address_timer_stop);
	nrf_timer_publish_set(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_EVENT_COMPARE0,
			      timer_compare0_radio_disable);

	nrf_timer_subscribe_set(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_TASK_STOP,
				radio_address_timer_stop);

	nrf_radio_subscribe_set(NRF_RADIO, NRF_RADIO_TASK_DISABLE, timer_compare0_radio_disable);

	channels_mask = (BIT(radio_address_timer_stop) |
			 BIT(timer_compare0_radio_disable));

	nrf_dppi_channels_enable(ESB_DPPIC, channels_mask);
}

void esb_ppi_for_wait_for_ack_clear(void)
{
	uint32_t channels_mask;

	channels_mask = (BIT(radio_address_timer_stop) |
			 BIT(timer_compare0_radio_disable));

	nrf_dppi_channels_disable(ESB_DPPIC, channels_mask);

	nrf_radio_publish_clear(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS);
	nrf_timer_publish_clear(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_EVENT_COMPARE0);

	nrf_timer_subscribe_clear(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_TASK_STOP);

	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_DISABLE);
}

void esb_ppi_for_wait_for_rx_set(void)
{
	uint32_t channels_mask;

	nrf_radio_publish_set(NRF_RADIO, ESB_RADIO_EVENT_END, radio_end_timer_start);
	nrf_timer_subscribe_set(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_TASK_START,
				radio_end_timer_start);

	channels_mask = (BIT(radio_end_timer_start));

	nrf_dppi_channels_enable(ESB_DPPIC, channels_mask);
}

void esb_ppi_for_wait_for_rx_clear(void)
{
	uint32_t channels_mask;

	channels_mask = (BIT(radio_end_timer_start));

	nrf_dppi_channels_disable(ESB_DPPIC, channels_mask);

	nrf_radio_publish_clear(NRF_RADIO, ESB_RADIO_EVENT_END);
	nrf_timer_subscribe_clear(ESB_NRF_TIMER_INSTANCE, NRF_TIMER_TASK_START);
}

uint32_t esb_ppi_radio_disabled_get(void)
{
	return disabled_phy_end_egu;
}

int esb_ppi_init(void)
{
	int ch;
	uint32_t domain_id = nrfx_gppi_domain_id_get((uint32_t)ESB_DPPIC);

#if defined(ESB_DPPI_FIXED)

	radio_address_timer_stop = ESB_DPPI_FIRST_FIXED_CHANNEL + 0;
	timer_compare0_radio_disable = ESB_DPPI_FIRST_FIXED_CHANNEL + 1;
	timer_compare1_radio_txen = ESB_DPPI_FIRST_FIXED_CHANNEL + 2;
	disabled_phy_end_egu = ESB_DPPI_FIRST_FIXED_CHANNEL + 3;
	egu_timer_start = ESB_DPPI_FIRST_FIXED_CHANNEL + 4;
	egu_ramp_up = ESB_DPPI_FIRST_FIXED_CHANNEL + 5;
	radio_end_timer_start = ESB_DPPI_FIRST_FIXED_CHANNEL + 6;
	ramp_up_dppi_group = ESB_DPPI_FIRST_FIXED_GROUP + 0;

	ARG_UNUSED(err);

#else
	ch = nrfx_gppi_channel_alloc(domain_id, NULL);
	if (ch < 0) {
		goto error;
	}
	radio_address_timer_stop = (uint8_t)ch;

	ch = nrfx_gppi_channel_alloc(domain_id, NULL);
	if (ch < 0) {
		goto error;
	}
	timer_compare0_radio_disable = (uint8_t)ch;

	ch = nrfx_gppi_channel_alloc(domain_id, NULL);
	if (ch < 0) {
		goto error;
	}
	timer_compare1_radio_txen = (uint8_t)ch;

	ch = nrfx_gppi_channel_alloc(domain_id, NULL);
	if (ch < 0) {
		goto error;
	}
	disabled_phy_end_egu = (uint8_t)ch;

	ch = nrfx_gppi_channel_alloc(domain_id, NULL);
	if (ch < 0) {
		goto error;
	}
	egu_timer_start = (uint8_t)ch;

	ch = nrfx_gppi_channel_alloc(domain_id, NULL);
	if (ch < 0) {
		goto error;
	}
	egu_ramp_up = (uint8_t)ch;

	if (IS_ENABLED(CONFIG_ESB_NEVER_DISABLE_TX)) {
		ch = nrfx_gppi_channel_alloc(domain_id, NULL);
		if (ch < 0) {
			goto error;
		}
		radio_end_timer_start = (uint8_t)ch;
	}

	ch = nrfx_gppi_group_channel_alloc(domain_id, NULL);
	if (ch < 0) {
		LOG_ERR("gppi_group_alloc failed with: %d\n", ch);
		return ch;
	}
	ramp_up_dppi_group = (uint8_t)ch;

#endif /* defined(ESB_DPPI_FIXED) */

	nrf_radio_publish_set(NRF_RADIO, NRF_RADIO_EVENT_DISABLED, disabled_phy_end_egu);
	if (IS_ENABLED(CONFIG_ESB_FAST_SWITCHING)) {
		nrf_radio_publish_set(NRF_RADIO, NRF_RADIO_EVENT_PHYEND, disabled_phy_end_egu);
	}
	nrf_dppi_channels_enable(ESB_DPPIC, BIT(disabled_phy_end_egu));

	return 0;

#if !defined(ESB_DPPI_FIXED)
error:
	LOG_ERR("gppi_channel_alloc failed with: %d\n", ch);
	return -ENODEV;
#endif /* !defined(ESB_DPPI_FIXED) */
}

void esb_ppi_disable_all(void)
{
	uint32_t channels_mask = (BIT(egu_ramp_up) |
				  BIT(disabled_phy_end_egu) |
				  BIT(egu_timer_start) |
				  BIT(radio_address_timer_stop) |
				  BIT(timer_compare0_radio_disable) |
				  BIT(radio_end_timer_start) |
				  (IS_ENABLED(CONFIG_ESB_NEVER_DISABLE_TX) ?
					BIT(timer_compare1_radio_txen) : 0));

	nrf_dppi_channels_disable(ESB_DPPIC, channels_mask);
}

void esb_ppi_deinit(void)
{
	nrf_dppi_channels_disable(ESB_DPPIC, BIT(disabled_phy_end_egu));
	nrf_radio_publish_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);
	if (IS_ENABLED(CONFIG_ESB_FAST_SWITCHING)) {
		nrf_radio_publish_clear(NRF_RADIO, NRF_RADIO_EVENT_PHYEND);
	}

#if defined(ESB_DPPI_FIXED)

	ARG_UNUSED(err);

#else
	uint32_t domain_id = nrfx_gppi_domain_id_get((uint32_t)ESB_DPPIC);

	nrfx_gppi_channel_free(domain_id, radio_address_timer_stop);
	nrfx_gppi_channel_free(domain_id, timer_compare0_radio_disable);
	nrfx_gppi_channel_free(domain_id, timer_compare1_radio_txen);
	nrfx_gppi_channel_free(domain_id, disabled_phy_end_egu);
	nrfx_gppi_channel_free(domain_id, egu_timer_start);
	nrfx_gppi_channel_free(domain_id, egu_ramp_up);
	if (IS_ENABLED(CONFIG_ESB_NEVER_DISABLE_TX)) {
		nrfx_gppi_channel_free(domain_id, radio_end_timer_start);
	}

	nrfx_gppi_group_channel_free(domain_id, ramp_up_dppi_group);

#endif /* defined(ESB_DPPI_FIXED) */
}
