/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <mpsl_dppi_protocol_api.h>
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>
#include <hal/nrf_dppi.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_ppib.h>

#define GPIOTE_NODE DT_NODELABEL(gpiote20)

LOG_MODULE_REGISTER(mpsl_radio_pin_debug, CONFIG_MPSL_LOG_LEVEL);

static void bsim_bridge_radio_dppi_to_peri(uint8_t dppi_ch, uint8_t ppib_ch)
{
	nrf_dppi_channels_enable(NRF_DPPIC10, BIT(dppi_ch));
	nrf_dppi_channels_enable(NRF_DPPIC20, BIT(dppi_ch));
	nrf_ppib_subscribe_set(NRF_PPIB11, nrf_ppib_send_task_get(ppib_ch), dppi_ch);
	nrf_ppib_publish_set(NRF_PPIB21, nrf_ppib_receive_event_get(ppib_ch), dppi_ch);
}

static void bsim_gpiote_subscribe_task(NRF_GPIOTE_Type *gpiote, nrf_gpiote_task_t task,
				       uint8_t dppi_ch, uint8_t ppib_ch)
{
	bsim_bridge_radio_dppi_to_peri(dppi_ch, ppib_ch);
	nrf_gpiote_subscribe_set(gpiote, task, dppi_ch);
}

static int bsim_gpiote_pin_setup(NRF_GPIOTE_Type *gpiote, uint8_t ch, uint32_t pin)
{
	nrf_gpiote_task_configure(gpiote, ch, pin, NRF_GPIOTE_POLARITY_TOGGLE,
				  NRF_GPIOTE_INITIAL_VALUE_LOW);
	nrf_gpiote_task_enable(gpiote, ch);

	return 0;
}

static int m_ppi_config(NRF_GPIOTE_Type *gpiote, uint8_t ch_ready_dis, uint8_t ch_addr_end)
{
	mpsl_dppi_fixed_channels_set();

	bsim_gpiote_subscribe_task(gpiote, nrf_gpiote_set_task_get(ch_ready_dis),
				   MPSL_DPPI_RADIO_PUBLISH_READY_CHANNEL_IDX,
				   MPSL_DPPI_RADIO_PUBLISH_READY_CHANNEL_IDX);
	bsim_gpiote_subscribe_task(gpiote, nrf_gpiote_clr_task_get(ch_ready_dis),
				   MPSL_DPPI_RADIO_PUBLISH_DISABLED_CH_IDX,
				   MPSL_DPPI_RADIO_PUBLISH_DISABLED_CH_IDX);

	bsim_gpiote_subscribe_task(gpiote, nrf_gpiote_set_task_get(ch_addr_end),
				   MPSL_DPPI_RADIO_PUBLISH_ADDRESS_CHANNEL_IDX,
				   MPSL_DPPI_RADIO_PUBLISH_ADDRESS_CHANNEL_IDX);
	bsim_gpiote_subscribe_task(gpiote, nrf_gpiote_clr_task_get(ch_addr_end),
				   MPSL_DPPI_RADIO_PUBLISH_END_CHANNEL_IDX,
				   MPSL_DPPI_RADIO_PUBLISH_END_CHANNEL_IDX);
	/* On nRF54L the stack may trigger PHYEND instead of END for packet completion. */
	bsim_gpiote_subscribe_task(gpiote, nrf_gpiote_clr_task_get(ch_addr_end),
				   MPSL_DPPI_RADIO_PUBLISH_PHYEND_CHANNEL_IDX,
				   MPSL_DPPI_RADIO_PUBLISH_PHYEND_CHANNEL_IDX);

	return 0;
}

static int mpsl_radio_pin_debug_init(void)
{
	nrfx_gpiote_t *gpiote = &GPIOTE_NRFX_INST_BY_NODE(GPIOTE_NODE);
	uint8_t radio_ready_radio_disabled_gpiote_channel;
	uint8_t radio_address_radio_end_gpiote_channel;
	int err;

	if (!nrfx_gpiote_init_check(gpiote)) {
		err = nrfx_gpiote_init(gpiote, 0);
		if (err != 0 && err != -EALREADY) {
			LOG_ERR("Failed initializing GPIOTE");
			return err;
		}
	}

	if (nrfx_gpiote_channel_alloc(gpiote, &radio_ready_radio_disabled_gpiote_channel) != 0) {
		LOG_ERR("Failed allocating GPIOTE chan");
		return -ENOMEM;
	}

	if (nrfx_gpiote_channel_alloc(gpiote, &radio_address_radio_end_gpiote_channel) != 0) {
		LOG_ERR("Failed allocating GPIOTE chan");
		return -ENOMEM;
	}

	if (bsim_gpiote_pin_setup(gpiote->p_reg, radio_ready_radio_disabled_gpiote_channel,
				  CONFIG_MPSL_PIN_DEBUG_RADIO_READY_AND_DISABLED_PIN) != 0) {
		LOG_ERR("Failed configuring GPIOTE chan");
		return -ENOMEM;
	}

	if (bsim_gpiote_pin_setup(gpiote->p_reg, radio_address_radio_end_gpiote_channel,
				  CONFIG_MPSL_PIN_DEBUG_RADIO_ADDRESS_AND_END_PIN) != 0) {
		LOG_ERR("Failed configuring GPIOTE chan");
		return -ENOMEM;
	}

	if (m_ppi_config(gpiote->p_reg, radio_ready_radio_disabled_gpiote_channel,
			 radio_address_radio_end_gpiote_channel) != 0) {
		LOG_ERR("Failed configuring DPPI");
		return -EIO;
	}

	return 0;
}

SYS_INIT(mpsl_radio_pin_debug_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
