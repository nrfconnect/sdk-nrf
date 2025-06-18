/*
 * Copyright (c) Nordic Semiconductor ASA. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor ASA.
 * The use, copying, transfer or disclosure of such information is prohibited except by
 * express written agreement with Nordic Semiconductor ASA.
 */

/**
 * @file
 *   This file implements function for Front End Module control when the "simple gpio"
 * implementation of Front End Module is selected. It applies to devices with PPI.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <hal/nrf_ppi.h>
#include <hal/nrf_timer.h>
#include <nrf.h>
#include <nrf_peripherals.h>

#include "../../../../../dragoon/mpsl/libs/fem/src/mpsl_fem_abstract_interface.h"

#include "fem_psemi_config.h"
#include "mpsl_fem_psemi_pins.h"
#include "fem_psemi_common.h"
#include "power_model.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#define ppi_ch_id_set ppi_channels[0]
#define ppi_ch_id_clr ppi_channels[1]

#define PPI_INVALID_CHANNEL 0xFF /**< Default value for the PPI holder variable. */
#define PPI_INVALID_GROUP                                                                          \
	(nrf_ppi_channel_group_t)0xFF /**< Default value for the PPI group holder variable. */

extern fem_psemi_interface_config_t m_fem_interface_config;
static mpsl_fem_gpiote_pin_t m_pa_gpiote_pin;
static mpsl_fem_gpiote_pin_t m_lna_gpiote_pin;
static bool m_fem_configured; /**< Whether the Front End Module was successfully configured. */
/** PPI channel provided by the `override_ppi = true` functionality. */
static uint8_t m_ppi_channel_ext = PPI_INVALID_CHANNEL;

/** Holders for events provided by the @ref mpsl_fem_pa_configuration_set */
static const mpsl_fem_event_t *mp_pa_activate_event = NULL;
static const mpsl_fem_event_t *mp_pa_deactivate_event = NULL;

/** Holders for events provided by the @ref mpsl_fem_lna_configuration_set */
static const mpsl_fem_event_t *mp_lna_activate_event = NULL;
static const mpsl_fem_event_t *mp_lna_deactivate_event = NULL;

/** Holder for PPI group provided by the @ref mpsl_fem_abort_set */
static nrf_ppi_channel_group_t m_ppi_abort_group = PPI_INVALID_GROUP;

static void fem_psemi_enable(void);
static int32_t fem_psemi_disable(void);
static int32_t fem_psemi_pa_configuration_set(const mpsl_fem_event_t *const p_activate_event,
					      const mpsl_fem_event_t *const p_deactivate_event);
static int32_t fem_psemi_lna_configuration_set(const mpsl_fem_event_t *const p_activate_event,
					       const mpsl_fem_event_t *const p_deactivate_event);
static int32_t fem_psemi_pa_configuration_clear(void);
static int32_t fem_psemi_lna_configuration_clear(void);
static void fem_psemi_deactivate_now(mpsl_fem_functionality_t type);
static void fem_psemi_cleanup(void);
static void fem_psemi_lna_check_is_configured(int8_t *const p_gain);
static int32_t fem_psemi_abort_set(mpsl_subscribable_hw_event_t event, uint32_t group);
static int32_t fem_psemi_abort_extend(uint32_t channel_to_add, uint32_t group);
static int32_t fem_psemi_abort_reduce(uint32_t channel_to_remove, uint32_t group);
static int32_t fem_psemi_abort_clear(void);

static const mpsl_fem_interface_t m_fem_psemi_methods = {
	.p_caps_get = fem_psemi_caps_get,
	.p_enable = fem_psemi_enable,
	.p_disable = fem_psemi_disable,
	.p_tx_power_split = fem_psemi_tx_power_split,
	.p_pa_configuration_set = fem_psemi_pa_configuration_set,
	.p_lna_configuration_set = fem_psemi_lna_configuration_set,
	.p_pa_configuration_clear = fem_psemi_pa_configuration_clear,
	.p_lna_configuration_clear = fem_psemi_lna_configuration_clear,
	.p_deactivate_now = fem_psemi_deactivate_now,
	.p_cleanup = fem_psemi_cleanup,
	.p_pa_power_control_set = fem_psemi_pa_power_control_set,
	.p_lna_is_configured = fem_psemi_lna_check_is_configured,
	.p_abort_set = fem_psemi_abort_set,
	.p_abort_extend = fem_psemi_abort_extend,
	.p_abort_reduce = fem_psemi_abort_reduce,
	.p_abort_clear = fem_psemi_abort_clear,
};

/** Map the mask bits with the Compare Channels. */
static uint32_t get_first_available_compare_channel(uint8_t mask)
{
	if (mask & (1 << 0)) {
		return NRF_TIMER_CC_CHANNEL0;
	}
	if (mask & (1 << 1)) {
		return NRF_TIMER_CC_CHANNEL1;
	}
	if (mask & (1 << 2)) {
		return NRF_TIMER_CC_CHANNEL2;
	}
	if (mask & (1 << 3)) {
		return NRF_TIMER_CC_CHANNEL3;
	}
	assert(false);
	return 0;
}

/** Configure the event with the provided values. */
static int32_t event_configuration_set(const mpsl_fem_event_t *const p_event,
				       mpsl_fem_gpiote_pin_config_t *p_pin_config, bool activate,
				       uint32_t time_delay)
{
	uint32_t task_addr;
	uint8_t ppi_ch;
	assert(p_event);
	assert(p_pin_config);

	if (p_event->override_ppi) {
		assert(p_event->ppi_ch_id != PPI_INVALID_CHANNEL);
		if (m_ppi_channel_ext == PPI_INVALID_CHANNEL) {
			/* External PPI channel placeholder is free. */
			m_ppi_channel_ext = ppi_ch = p_event->ppi_ch_id;
		} else if ((m_ppi_channel_ext == p_event->ppi_ch_id) &&
			   (!NRF_PPI->FORK[(uint32_t)m_ppi_channel_ext].TEP)) {
			/* PPI is equal to the already set, but the one set has a free fork
			 * endpoint. */
			ppi_ch = p_event->ppi_ch_id;
		} else {
			return -NRF_EINVAL;
		}
	} else {
		ppi_ch = activate ? m_fem_interface_config.ppi_ch_id_set
				  : m_fem_interface_config.ppi_ch_id_clr;
	}

	task_addr = mpsl_fem_gpiote_pin_config_action_task_address_get(p_pin_config, activate);

	switch (p_event->type) {
	case MPSL_FEM_EVENT_TYPE_GENERIC: {
		if (NRF_PPI->CH[(uint32_t)ppi_ch].TEP) {
			nrf_ppi_fork_endpoint_setup(NRF_PPI, (nrf_ppi_channel_t)ppi_ch, task_addr);
		} else {
			nrf_ppi_channel_endpoint_setup(NRF_PPI, (nrf_ppi_channel_t)ppi_ch,
						       p_event->event.generic.event, task_addr);
		}

		nrf_ppi_channel_enable(NRF_PPI, (nrf_ppi_channel_t)ppi_ch);
	} break;

	case MPSL_FEM_EVENT_TYPE_TIMER: {
		assert(p_event->event.timer.compare_channel_mask);

		uint32_t compare_channel = get_first_available_compare_channel(
			p_event->event.timer.compare_channel_mask);

		nrf_ppi_channel_endpoint_setup(
			NRF_PPI, (nrf_ppi_channel_t)ppi_ch,
			(uint32_t)(&(p_event->event.timer.p_timer_instance
					     ->EVENTS_COMPARE[compare_channel])),
			task_addr);
		nrf_ppi_channel_enable(NRF_PPI, (nrf_ppi_channel_t)ppi_ch);

		nrf_timer_cc_set(p_event->event.timer.p_timer_instance,
				 (nrf_timer_cc_channel_t)compare_channel,
				 p_event->event.timer.counter_period.end - time_delay);
	} break;

	default:
		assert(false);
		break;
	}

	return 0;
}

/** Deconfigure the event with the provided values. */
static int32_t event_configuration_clear(const mpsl_fem_event_t *const p_event, bool activate)
{
	uint8_t ppi_ch;
	assert(p_event);

	if (p_event->override_ppi) {
		ppi_ch = p_event->ppi_ch_id;
	} else {
		ppi_ch = activate ? m_fem_interface_config.ppi_ch_id_set
				  : m_fem_interface_config.ppi_ch_id_clr;
	}

	nrf_ppi_channel_disable(NRF_PPI, (nrf_ppi_channel_t)ppi_ch);
	nrf_ppi_channel_endpoint_setup(NRF_PPI, (nrf_ppi_channel_t)ppi_ch, 0, 0);
	nrf_ppi_fork_endpoint_setup(NRF_PPI, (nrf_ppi_channel_t)ppi_ch, 0);

	switch (p_event->type) {
	case MPSL_FEM_EVENT_TYPE_GENERIC:
		break;

	case MPSL_FEM_EVENT_TYPE_TIMER:
		break;

	default:
		assert(false);
		break;
	}

	return 0;
}

static void fem_psemi_enable(void)
{
}

static int32_t fem_psemi_disable(void)
{
	if (mp_pa_activate_event || mp_pa_deactivate_event || mp_lna_activate_event ||
	    mp_lna_deactivate_event) {
		return -NRF_EPERM;
	}

	// Intentionally do nothing in this function
	// Power down mode is not supported

	return 0;
}

static int32_t fem_psemi_pa_configuration_set(const mpsl_fem_event_t *const p_activate_event,
					      const mpsl_fem_event_t *const p_deactivate_event)
{
	int32_t ret_code;

	if (!(m_fem_interface_config.pa_pin_config.enable)) {
		return -NRF_EPERM;
	}

	if (p_activate_event) {
		ret_code = event_configuration_set(
			p_activate_event, &m_fem_interface_config.pa_pin_config, true,
			m_fem_interface_config.fem_config.pa_time_gap_us);
		if (ret_code != 0) {
			return ret_code;
		}

		mp_pa_activate_event = p_activate_event;
	}

	if (p_deactivate_event) {
		ret_code = event_configuration_set(
			p_deactivate_event, &m_fem_interface_config.pa_pin_config, false,
			m_fem_interface_config.fem_config.pa_time_gap_us);
		if (ret_code != 0) {
			return ret_code;
		}

		mp_pa_deactivate_event = p_deactivate_event;
	}

	return 0;
}

static int32_t fem_psemi_lna_configuration_set(const mpsl_fem_event_t *const p_activate_event,
					       const mpsl_fem_event_t *const p_deactivate_event)
{
	int32_t ret_code;

	if (!(m_fem_interface_config.lna_pin_config.enable)) {
		return -NRF_EPERM;
	}

	if (p_activate_event) {
		ret_code = event_configuration_set(
			p_activate_event, &m_fem_interface_config.lna_pin_config, true,
			m_fem_interface_config.fem_config.lna_time_gap_us);
		if (ret_code != 0) {
			return ret_code;
		}

		mp_lna_activate_event = p_activate_event;
	}

	if (p_deactivate_event) {
		ret_code = event_configuration_set(
			p_deactivate_event, &m_fem_interface_config.lna_pin_config, false,
			m_fem_interface_config.fem_config.lna_time_gap_us);
		if (ret_code != 0) {
			return ret_code;
		}

		mp_lna_deactivate_event = p_deactivate_event;
	}

	return 0;
}

static int32_t fem_psemi_pa_configuration_clear(void)
{
	int32_t ret_code;

	if (!(m_fem_interface_config.pa_pin_config.enable)) {
		return -NRF_EPERM;
	}

	fem_psemi_pa_gain_default(&m_fem_interface_config);

	if (mp_pa_activate_event) {
		ret_code = event_configuration_clear(mp_pa_activate_event, true);
		if (ret_code != 0) {
			return ret_code;
		}

		mp_pa_activate_event = NULL;
	}

	if (mp_pa_deactivate_event) {
		ret_code = event_configuration_clear(mp_pa_deactivate_event, false);
		if (ret_code != 0) {
			return ret_code;
		}

		mp_pa_deactivate_event = NULL;
	}

	m_ppi_channel_ext = PPI_INVALID_CHANNEL;

	return 0;
}

static int32_t fem_psemi_lna_configuration_clear(void)
{
	int32_t ret_code;

	if (!(m_fem_interface_config.lna_pin_config.enable)) {
		return -NRF_EPERM;
	}

	if (mp_lna_activate_event) {
		ret_code = event_configuration_clear(mp_lna_activate_event, true);
		if (ret_code != 0) {
			return ret_code;
		}

		mp_lna_activate_event = NULL;
	}

	if (mp_lna_deactivate_event) {
		ret_code = event_configuration_clear(mp_lna_deactivate_event, false);
		if (ret_code != 0) {
			return ret_code;
		}

		mp_lna_deactivate_event = NULL;
	}

	m_ppi_channel_ext = PPI_INVALID_CHANNEL;

	return 0;
}

static void fem_psemi_deactivate_now(mpsl_fem_functionality_t type)
{
	if (m_fem_interface_config.pa_pin_config.enable && (type & MPSL_FEM_PA)) {
		mpsl_fem_gpiote_pin_config_output_active_write(
			&m_fem_interface_config.pa_pin_config, false);
	}

	if (m_fem_interface_config.lna_pin_config.enable && (type & MPSL_FEM_LNA)) {
		mpsl_fem_gpiote_pin_config_output_active_write(
			&m_fem_interface_config.lna_pin_config, false);
	}
}

int32_t fem_psemi_interface_config_set(fem_psemi_interface_config_t const *const p_config)
{
	int32_t ret_code;
	m_fem_interface_config = *p_config;

#if defined(CONFIG_POWER_MAP_MODEL)
	power_model_init();
#endif

	mpsl_fem_gpiote_pin_init(&m_pa_gpiote_pin, &p_config->pa_pin_config);
	mpsl_fem_gpiote_pin_init(&m_lna_gpiote_pin, &p_config->lna_pin_config);

	fem_psemi_gain_gpio_configure(p_config);

	ret_code = mpsl_fem_interface_set(&m_fem_psemi_methods);

	if (ret_code != 0) {
		return ret_code;
	}

	m_fem_configured = true;

	return 0;
}

int32_t fem_psemi_interface_config_get(fem_psemi_interface_config_t *const p_config)
{
	if (p_config == NULL) {
		return -NRF_EPERM;
	}

	if (m_fem_configured) {
		*p_config = m_fem_interface_config;
		return 0;
	} else {
		return -NRF_EPERM;
	}
}

static void fem_psemi_cleanup(void)
{
	nrf_ppi_channel_disable(NRF_PPI, (nrf_ppi_channel_t)m_fem_interface_config.ppi_ch_id_set);
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI, (nrf_ppi_channel_t)m_fem_interface_config.ppi_ch_id_set, 0, 0);
	nrf_ppi_fork_endpoint_setup(NRF_PPI,
				    (nrf_ppi_channel_t)m_fem_interface_config.ppi_ch_id_set, 0);
	nrf_ppi_channel_disable(NRF_PPI, (nrf_ppi_channel_t)m_fem_interface_config.ppi_ch_id_clr);
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI, (nrf_ppi_channel_t)m_fem_interface_config.ppi_ch_id_clr, 0, 0);
	nrf_ppi_fork_endpoint_setup(NRF_PPI,
				    (nrf_ppi_channel_t)m_fem_interface_config.ppi_ch_id_clr, 0);
	if (m_ppi_channel_ext != PPI_INVALID_CHANNEL) {
		nrf_ppi_channel_disable(NRF_PPI, (nrf_ppi_channel_t)m_ppi_channel_ext);
		nrf_ppi_channel_endpoint_setup(NRF_PPI, (nrf_ppi_channel_t)m_ppi_channel_ext, 0, 0);
		nrf_ppi_fork_endpoint_setup(NRF_PPI, (nrf_ppi_channel_t)m_ppi_channel_ext, 0);
		m_ppi_channel_ext = PPI_INVALID_CHANNEL;
	}
}

static int32_t fem_psemi_abort_set(mpsl_subscribable_hw_event_t event, uint32_t group)
{
	int32_t ret_code;

	mpsl_fem_event_t abort_event;
	abort_event.type = MPSL_FEM_EVENT_TYPE_GENERIC;
	abort_event.override_ppi = false;
	abort_event.event.generic.event = event;

	ret_code = event_configuration_set(&abort_event, &m_fem_interface_config.pa_pin_config,
					   false, m_fem_interface_config.fem_config.pa_time_gap_us);
	if (ret_code != 0) {
		return ret_code;
	}

	ret_code =
		event_configuration_set(&abort_event, &m_fem_interface_config.lna_pin_config, false,
					m_fem_interface_config.fem_config.lna_time_gap_us);
	if (ret_code != 0) {
		return ret_code;
	}

	m_ppi_abort_group = (nrf_ppi_channel_group_t)group;
	return 0;
}

static int32_t fem_psemi_abort_extend(uint32_t channel_to_add, uint32_t group)
{
	(void)channel_to_add;
	(void)group;
	/* in this Front End Module implementation dont have to do anything here */
	return 0;
}

static int32_t fem_psemi_abort_reduce(uint32_t channel_to_remove, uint32_t group)
{
	(void)channel_to_remove;
	(void)group;
	/* in this Front End Module implementation dont have to do anything here */
	return 0;
}

static int32_t fem_psemi_abort_clear(void)
{
	int32_t ret;

	static const mpsl_fem_event_t abort_event = {.type = MPSL_FEM_EVENT_TYPE_GENERIC,
						     .override_ppi = false,
						     .event.generic.event = 0U};

	nrf_ppi_group_disable(NRF_PPI, m_ppi_abort_group);
	nrf_ppi_group_clear(NRF_PPI, m_ppi_abort_group);
	ret = event_configuration_clear(&abort_event, false);
	assert(ret == 0);

	return 0;
}

static void fem_psemi_lna_check_is_configured(int8_t *const p_gain)
{
	*p_gain = m_fem_interface_config.fem_config.lna_gain_db;
}
