/* Copyright (c) 2024, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 *   This file implements Front End Module control. It applies to nRF54 devices.
 */

/*
 * <event> is of mpsl_subscribable_hw_event_t type, it is a dppi already, so notation
 * <event>.dppi is used below. Arrows point from event's PUBLISH/task's SUBSCRIBE registers
 * to dppi channel.
 *
 * <abort_event> is of mpsl_subscribable_hw_event_t and is set by fem_simple_gpio_abort_set
 * function. On diagrams below `group` parameter of fem_simple_gpio_abort_set is ignored (not
 * supported).
 *
 * TIMER's CC numbers denoted by letters CC[a], CC[b] are not particular.
 *
 * GPIOTE[n].TASK_<on> is GPIOTE n-th channel's task to make given pin "active". What means
 * "active" for a given pin depends on `mpsl_fem_gpiote_pin_config_t::active_high` setting.
 *
 * 1) RXEN activation on TIMER, deactivation on <event> (<abort_event> not set):
 * Covers following sequence of configuration:
 *     - mpsl_fem_lna_configuration_set( "on timer", "on event `event`")
 *
 * Notes: Used in SD controller.
 *
 * Connections:
 * TIMER.CC[a] ---------------------------------> dppi0 <-- GPIOTE[pin_rxen].TASK_<on>
 *
 * <event>.dppi <------------------------------------------ GPIOTE[pin_rxen].TASK_<off>
 *
 * 2) TXEN activation on TIMER, deactivation on <event> (<abort_event> not set):
 * Covers following sequence of configuration:
 *     - mpsl_fem_pa_configuration_set( "on timer", "on event `event`")
 *
 * Notes: Used in SD controller.
 *
 * Connections:
 * TIMER.CC[a] ---------------------------------> dppi0 <-- GPIOTE[pin_txen].TASK_<on>
 *
 * <event>.dppi <------------------------------------------ GPIOTE[pin_txen].TASK_<off>
 *
 * 3.a) RXEN and TXEN deactivation on <abort_event> only:
 * Covers following sequence of configuration:
 *     - mpsl_fem_abort_set( "on `abort_event`", group=ignore)
 *
 * Notes: Used in nRF 802.15.4 Radio Driver. Connections-2 are considered optimal,
 * however this configuration is only temporary, and after additional calls
 * GPIOTE[pin_rxen].TASK_<off> will have multiple sources. Thus egu is used.
 * This egu is visible in other configurations as well.
 *
 * Connections:
 * <abort_event>.dppi <---+---- egu1 ---> dppi1 <-- GPIOTE[pin_rxen].TASK_<off>
 *                        |
 *                        +------------------------ GPIOTE[pin_txen].TASK_<off>
 *
 * Connections-2:
 * <abort_event>.dppi <---+------------------------ GPIOTE[pin_rxen].TASK_<off>
 *                        |
 *                        +------------------------ GPIOTE[pin_txen].TASK_<off>
 *
 * 3.b) RXEN activation on TIMER, deactivation on <abort_event> only
 * Covers following sequence of configuration:
 *     - mpsl_fem_abort_set( "on `abort_event`", group=ignore)
 *     - mpsl_fem_lna_configuration_set( "on timer", NULL)
 *
 * Notes: Used in nRF 802.15.4 Radio Driver, receive operation.
 *
 * Connections:
 * TIMER.CC[a] ---------------------------------> dppi0 <-- GPIOTE[pin_rxen].TASK_<on>
 *
 * <abort_event>.dppi <---+------------ egu1 ---> dppi1 <-- GPIOTE[pin_rxen].TASK_<off>
 *                        |
 *                        +-------------------------------- GPIOTE[pin_txen].TASK_<off>
 *
 * 3.c) RXEN activation on TIMER, deactivation on <event> or <abort_event>:
 * Covers following sequence of configuration:
 *     - mpsl_fem_abort_set( "on `abort_event`", group=ignore)
 *     - mpsl_fem_lna_configuration_set( "on timer", "on event `event`")
 *
 * Notes: Used in nRF 802.15.4 Radio Driver as intermediate step to achieve case 3.f)
 *
 * Connections:
 * TIMER.CC[a] ---------------------------------> dppi0 <-- GPIOTE[pin_rxen].TASK_<on>
 *
 * <event>.dppi <------------- egu0 --------+---> dppi1 <-- GPIOTE[pin_rxen].TASK_<off>
 *                                          |
 * <abort_event>.dppi <---+--- egu1 --------+
 *                        |
 *                        +-------------------------------- GPIOTE[pin_txen].TASK_<off>
 *
 * 3.d) TXEN activation on TIMER, deactivation on <abort_event> only
 * Covers following sequence of configuration:
 *     - mpsl_fem_abort_set( "on `abort_event`", group=ignore)
 *     - mpsl_fem_pa_configuration_set( "on timer", NULL )
 *
 * Notes: Used in nRF 802.15.4 Radio Driver, transmit operation.
 *
 * Connections:
 * TIMER.CC[a] ---------------------------------> dppi0 <-- GPIOTE[pin_txen].TASK_<on>
 *
 * <abort_event>.dppi <---+-------------------------------- GPIOTE[pin_txen].TASK_<off>
 *                        |
 *                        +------------ egu1 ---> dppi1 <-- GPIOTE[pin_rxen].TASK_<off>
 *
 * 3.e) TXEN activation on TIMER, deactivation on <event> or <abort_event>:
 * Covers following sequence of configuration:
 *     - mpsl_fem_abort_set( "on `abort_event`", group=ignore)
 *     - mpsl_fem_pa_configuration_set( "on timer", "on event")
 *
 * Notes: Not used
 *
 * 3.f) RXEN activation on TIMER, deactivation on <event> or <abort_event>,
 *      TXEN activation on <event> (the same as RXEN deactivation) deactivation on <abort_event>
 *      only.
 *     - mpsl_fem_abort_set( "on `abort_event`", group=ignore)
 *     - mpsl_fem_lna_configuration_set( "on timer", "on event `event`")
 *     - mpsl_fem_pa_configuration_set( "on event `event`", NULL)
 *
 * Connections:
 * TIMER.CC[a] ---------------------------------> dppi0 <-- GPIOTE[pin_rxen].TASK_<on>
 *
 * <event>.dppi <-----------+--- egu0 -----+----> dppi1 <-- GPIOTE[pin_rxen].TASK_<off>
 *                          |              |
 * <abort_event>.dppi <--+--|--- egu1 -----+
 *                       |  |
 *                       |  +----egu2 ----------> dppi2 <-- GPIOTE[pin_txen].TASK_<on>
 *                       |
 *                       +--------------------------------- GPIOTE[pin_txen].TASK_<off>
 */

#include <assert.h>
#include <nrf.h>
#include <hal/nrf_dppi.h>
#include <hal/nrf_egu.h>
#include <hal/nrf_timer.h>
#include <hal/nrf_radio.h>

#include "../../../../../dragoon/mpsl/libs/fem/src/mpsl_fem_abstract_interface.h"

#include "fem_psemi_config.h"
#include "mpsl_fem_psemi_pins.h"
#include "fem_psemi_common.h"
#include "power_model.h"

extern uint8_t mpsl_fem_utils_available_cc_channel_get(uint8_t mask, uint32_t number);
extern fem_psemi_interface_config_t m_fem_interface_config;

#define M_EGU0 (m_fem_interface_config.egu_channels[0])
#define M_EGU1 (m_fem_interface_config.egu_channels[1])
#define M_EGU2 (m_fem_interface_config.egu_channels[2])

#define M_DPPI0 (m_fem_interface_config.dppi_channels[0])
#define M_DPPI1 (m_fem_interface_config.dppi_channels[1])
#define M_DPPI2 (m_fem_interface_config.dppi_channels[2])

static bool m_abort_event_is_set;
static mpsl_fem_gpiote_pin_t m_pa_gpiote_pin;
static mpsl_fem_gpiote_pin_t m_lna_gpiote_pin;

/** Holders for events provided by the @ref mpsl_fem_pa_configuration_set */
static const mpsl_fem_event_t *mp_pa_activate_event = NULL;
static const mpsl_fem_event_t *mp_pa_deactivate_event = NULL;

/** Holders for events provided by the @ref mpsl_fem_lna_configuration_set */
/** LNA is deactivated by NRF_802154_DPPI_RADIO_DISABLED dppi set by mpsl_fem_abort_set. */
static const mpsl_fem_event_t *mp_lna_activate_event = NULL;
static const mpsl_fem_event_t *mp_lna_deactivate_event = NULL;

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

static void fem_psemi_enable(void)
{
}

static void activation_on_timer_set(const mpsl_fem_event_t *const p_event,
				    mpsl_fem_gpiote_pin_t *p_gpiote_pin, bool gpiote_pin_level,
				    uint8_t dppi_ch, uint32_t time_delay, uint32_t v_cc_no)
{
	uint32_t cc_no = mpsl_fem_utils_available_cc_channel_get(
		p_event->event.timer.compare_channel_mask, v_cc_no);
	assert(cc_no != UINT8_MAX);

	nrf_timer_publish_set(p_event->event.timer.p_timer_instance,
			      nrf_timer_compare_event_get((uint8_t)cc_no), dppi_ch);
	mpsl_fem_gpiote_pin_task_subscribe_set(p_gpiote_pin, gpiote_pin_level, dppi_ch);

	/* Start with the smallest robustly triggered target time. */
	uint32_t target_time = 1;
	if (p_event->event.timer.counter_period.end > time_delay) {
		/* Update target time so that the activation finishes exactly
		 * at the end of the specified period. */
		target_time = p_event->event.timer.counter_period.end - time_delay;
	}

	nrf_timer_cc_set(p_event->event.timer.p_timer_instance, (nrf_timer_cc_channel_t)cc_no,
			 target_time);

	nrf_dppi_channels_enable(NRF_DPPIC10, 1U << dppi_ch);
}

static void activation_on_timer_clear(const mpsl_fem_event_t *const p_event,
				      mpsl_fem_gpiote_pin_t *p_gpiote_pin, bool gpiote_pin_level,
				      uint8_t dppi_ch, uint32_t v_cc_no)
{
	nrf_dppi_channels_disable(NRF_DPPIC10, (1U << dppi_ch));

	uint32_t cc_no = mpsl_fem_utils_available_cc_channel_get(
		p_event->event.timer.compare_channel_mask, v_cc_no);
	assert(cc_no != UINT8_MAX);

	mpsl_fem_gpiote_pin_task_subscribe_clear(p_gpiote_pin, gpiote_pin_level);

	nrf_timer_publish_clear(p_event->event.timer.p_timer_instance,
				nrf_timer_compare_event_get((uint8_t)cc_no));
}

static int32_t fem_psemi_disable(void)
{
	if (mp_pa_activate_event || mp_pa_deactivate_event || mp_lna_activate_event ||
	    mp_lna_deactivate_event) {
		// Actions on PA/LNA pins are still configured. Cannot disable
		return -NRF_EPERM;
	}

	return 0;
}

static int32_t pa_activation_on_generic_set(const mpsl_fem_event_t *const p_event)
{
	if (!mpsl_fem_gpiote_pin_is_enabled(&m_pa_gpiote_pin)) {
		return -NRF_EPERM;
	}

	// Case 3.f) of the doc
	nrf_egu_subscribe_set(NRF_EGU10, nrf_egu_trigger_task_get(M_EGU2),
			      p_event->event.generic.event);
	nrf_egu_publish_set(NRF_EGU10, nrf_egu_triggered_event_get(M_EGU2), M_DPPI2);
	mpsl_fem_gpiote_pin_task_subscribe_set(&m_pa_gpiote_pin, true, M_DPPI2);

	nrf_dppi_channels_enable(NRF_DPPIC10, (1U << M_DPPI2));

	return 0;
}

static void pa_activation_on_generic_clear(void)
{
	if (!mpsl_fem_gpiote_pin_is_enabled(&m_pa_gpiote_pin)) {
		return;
	}

	nrf_dppi_channels_disable(NRF_DPPIC10, (1U << M_DPPI2));

	mpsl_fem_gpiote_pin_task_subscribe_clear(&m_pa_gpiote_pin, true);

	nrf_egu_publish_clear(NRF_EGU10, nrf_egu_triggered_event_get(M_EGU2));

	nrf_egu_subscribe_clear(NRF_EGU10, nrf_egu_trigger_task_get(M_EGU2));
}

static int32_t deactivation_on_generic_set(const mpsl_fem_event_t *const p_event,
					   mpsl_fem_gpiote_pin_t *p_gpiote_pin,
					   bool gpiote_pin_level)
{
	if (!mpsl_fem_gpiote_pin_is_enabled(p_gpiote_pin)) {
		return -NRF_EPERM;
	}

	if (m_abort_event_is_set) {
		if (!gpiote_pin_level && (p_gpiote_pin == &m_lna_gpiote_pin)) {
			// Case 3.c) 3.f)
			/* After fem_simple_gpio_abort_set is called, following holds:
			 * gpiote_task is subscribed to M_DPPI1
			 * M_DPPI1 is enabled
			 * Any number of tasks can publish to M_DPPI1 to trigger gpiote_task
			 */
			nrf_egu_subscribe_set(NRF_EGU10, nrf_egu_trigger_task_get(M_EGU0),
					      p_event->event.generic.event);
			nrf_egu_publish_set(NRF_EGU10, nrf_egu_triggered_event_get(M_EGU0),
					    M_DPPI1);
		} else {
			// Other cases, including 3.e) not supported, would require additional egu
			return -NRF_EINVAL;
		}

	} else {
		// Case 1) 2) of the doc.
		mpsl_fem_gpiote_pin_task_subscribe_set(p_gpiote_pin, gpiote_pin_level,
						       p_event->event.generic.event);
	}

	return 0;
}

static void deactivation_on_generic_clear(mpsl_fem_gpiote_pin_t *p_gpiote_pin,
					  bool gpiote_pin_level)
{
	if (!mpsl_fem_gpiote_pin_is_enabled(p_gpiote_pin)) {
		return;
	}

	if (m_abort_event_is_set) {
		if (!gpiote_pin_level && (p_gpiote_pin == &m_lna_gpiote_pin)) {
			// Case 3.c) 3.f)
			nrf_egu_publish_clear(NRF_EGU10, nrf_egu_triggered_event_get(M_EGU0));
			nrf_egu_subscribe_clear(NRF_EGU10, nrf_egu_trigger_task_get(M_EGU0));
		} else {
			// Other cases unsupported, more egu would be needed
		}
	} else {
		// Case 1) 2) of the doc.
		mpsl_fem_gpiote_pin_task_subscribe_clear(p_gpiote_pin, gpiote_pin_level);
	}
}

static int32_t fem_psemi_pa_configuration_set(const mpsl_fem_event_t *const p_activate_event,
					      const mpsl_fem_event_t *const p_deactivate_event)
{
	int32_t ret_val = 0;

	if (fem_psemi_state_get() != FEM_PSEMI_STATE_AUTO) {
		return -NRF_EPERM;
	}

	if (p_activate_event != NULL) {
		switch (p_activate_event->type) {
		case MPSL_FEM_EVENT_TYPE_TIMER:
			activation_on_timer_set(p_activate_event, &m_pa_gpiote_pin, true, M_DPPI0,
						m_fem_interface_config.fem_config.pa_time_gap_us,
						0);
			break;

		case MPSL_FEM_EVENT_TYPE_GENERIC:
			ret_val = pa_activation_on_generic_set(p_activate_event);
			break;

		default:
			ret_val = -NRF_EINVAL;
			break;
		}

		if (ret_val != 0) {
			return ret_val;
		}

		mp_pa_activate_event = p_activate_event;
	}

	if (p_deactivate_event != NULL) {
		switch (p_deactivate_event->type) {
		case MPSL_FEM_EVENT_TYPE_GENERIC:
			ret_val = deactivation_on_generic_set(p_deactivate_event, &m_pa_gpiote_pin,
							      false);
			break;

		default:
			ret_val = -NRF_EINVAL;
			break;
		}

		if (ret_val != 0) {
			return ret_val;
		}

		mp_pa_deactivate_event = p_deactivate_event;
	}

	return 0;
}

static int32_t fem_psemi_lna_configuration_set(const mpsl_fem_event_t *const p_activate_event,
					       const mpsl_fem_event_t *const p_deactivate_event)
{
	int32_t ret_val = 0;

	if (fem_psemi_state_get() != FEM_PSEMI_STATE_AUTO) {
		return -NRF_EPERM;
	}

	if (p_activate_event != NULL) {
		switch (p_activate_event->type) {
		case MPSL_FEM_EVENT_TYPE_TIMER:
			activation_on_timer_set(p_activate_event, &m_lna_gpiote_pin, true, M_DPPI0,
						m_fem_interface_config.fem_config.lna_time_gap_us,
						0);
			break;

		case MPSL_FEM_EVENT_TYPE_GENERIC:
			/* There is no support for lna activation by event */
			/* fallthrough */
		default:
			ret_val = -NRF_EINVAL;
			break;
		}

		if (ret_val != 0) {
			return ret_val;
		}

		mp_lna_activate_event = p_activate_event;
	}

	if (p_deactivate_event != NULL) {
		switch (p_deactivate_event->type) {
		case MPSL_FEM_EVENT_TYPE_GENERIC:
			ret_val = deactivation_on_generic_set(p_deactivate_event, &m_lna_gpiote_pin,
							      false);
			break;

		case MPSL_FEM_EVENT_TYPE_TIMER:
			/* There is no support for lna deactivation by timer */
			/* fallthrough */

		default:
			ret_val = -NRF_EINVAL;
			break;
		}

		if (ret_val != 0) {

			return ret_val;
		}

		mp_lna_deactivate_event = p_deactivate_event;
	}

	return 0;
}

static int32_t fem_psemi_pa_configuration_clear(void)
{
	if (fem_psemi_state_get() != FEM_PSEMI_STATE_AUTO) {
		return -NRF_EPERM;
	}

	fem_psemi_pa_gain_default(&m_fem_interface_config);

	if (mp_pa_activate_event != NULL) {
		switch (mp_pa_activate_event->type) {
		case MPSL_FEM_EVENT_TYPE_TIMER:
			activation_on_timer_clear(mp_pa_activate_event, &m_pa_gpiote_pin, true,
						  M_DPPI0, 0);
			break;

		case MPSL_FEM_EVENT_TYPE_GENERIC:
			pa_activation_on_generic_clear();
			break;

		default:
			assert(false);
			return -NRF_EINVAL;
		}

		mp_pa_activate_event = NULL;
	}

	if (mp_pa_deactivate_event != NULL) {
		switch (mp_pa_deactivate_event->type) {
		case MPSL_FEM_EVENT_TYPE_GENERIC:
			deactivation_on_generic_clear(&m_pa_gpiote_pin, false);
			break;
		default:
			assert(false);
			return -NRF_EINVAL;
		}
		mp_pa_deactivate_event = NULL;
	}

	return 0;
}

static int32_t fem_psemi_lna_configuration_clear(void)
{
	if (!(m_fem_interface_config.lna_pin_config.enable)) {
		return -NRF_EPERM;
	}

	if (mp_lna_activate_event != NULL) {
		switch (mp_lna_activate_event->type) {
		case MPSL_FEM_EVENT_TYPE_TIMER:
			activation_on_timer_clear(mp_lna_activate_event, &m_lna_gpiote_pin, true,
						  M_DPPI0, 0);
			break;

		case MPSL_FEM_EVENT_TYPE_GENERIC:
			/* There is no support for lna activation by event, so it should
			 * not be possible to save such event into mp_lna_activate_event.
			 */
			/* fallthrough */
		default:
			assert(false);
			return -NRF_EINVAL;
		}

		mp_lna_activate_event = NULL;
	}

	if (mp_lna_deactivate_event != NULL) {
		switch (mp_lna_deactivate_event->type) {
		case MPSL_FEM_EVENT_TYPE_GENERIC:
			deactivation_on_generic_clear(&m_lna_gpiote_pin, false);
			break;
		default:
			assert(false);
			return -NRF_EINVAL;
		}
		mp_lna_deactivate_event = NULL;
	}

	return 0;
}

static void fem_psemi_deactivate_now(mpsl_fem_functionality_t type)
{
	if (mpsl_fem_gpiote_pin_is_enabled(&m_pa_gpiote_pin) && (type & MPSL_FEM_PA)) {
		mpsl_fem_gpiote_pin_output_active_write(&m_pa_gpiote_pin, false);
	}

	if (mpsl_fem_gpiote_pin_is_enabled(&m_lna_gpiote_pin) && (type & MPSL_FEM_LNA)) {
		mpsl_fem_gpiote_pin_output_active_write(&m_lna_gpiote_pin, false);
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

	return 0;
}

static void fem_psemi_cleanup(void)
{
	(void)fem_psemi_pa_configuration_clear();
	(void)fem_psemi_lna_configuration_clear();
	(void)fem_psemi_abort_clear();
}

static int32_t fem_psemi_abort_set(mpsl_subscribable_hw_event_t event, uint32_t group)
{
    // Case 3.a) of the doc.

    (void)group;

    if ((mp_pa_activate_event != NULL) || (mp_lna_activate_event != NULL))
    {
        return -NRF_EINVAL;
    }

	if (fem_psemi_state_get() != FEM_PSEMI_STATE_AUTO) {
		return -NRF_EPERM;
	}

    if (mpsl_fem_gpiote_pin_is_enabled(&m_lna_gpiote_pin))
    {
        nrf_egu_subscribe_set(NRF_EGU10, nrf_egu_trigger_task_get(M_EGU1), event);
        nrf_egu_publish_set(NRF_EGU10, nrf_egu_triggered_event_get(M_EGU1), M_DPPI1);
        mpsl_fem_gpiote_pin_task_subscribe_set(&m_lna_gpiote_pin, false, M_DPPI1);
        nrf_dppi_channels_enable(NRF_DPPIC10, (1U << M_DPPI1));
    }

    if (mpsl_fem_gpiote_pin_is_enabled(&m_pa_gpiote_pin))
    {
        mpsl_fem_gpiote_pin_task_subscribe_set(&m_pa_gpiote_pin, false, event);
    }

    m_abort_event_is_set = true;

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
    if (!m_abort_event_is_set)
    {
        // There's nothing to clear.
        return -NRF_EPERM;
    }

	if (fem_psemi_state_get() != FEM_PSEMI_STATE_AUTO) {
		return -NRF_EPERM;
	}

    if (mpsl_fem_gpiote_pin_is_enabled(&m_lna_gpiote_pin))
    {
        nrf_dppi_channels_disable(NRF_DPPIC10, (1U << M_DPPI1));
        mpsl_fem_gpiote_pin_task_subscribe_clear(&m_lna_gpiote_pin, false);
        nrf_egu_publish_clear(NRF_EGU10, nrf_egu_triggered_event_get(M_EGU1));
        nrf_egu_subscribe_clear(NRF_EGU10, nrf_egu_trigger_task_get(M_EGU1));
    }
    if (mpsl_fem_gpiote_pin_is_enabled(&m_pa_gpiote_pin))
    {
        mpsl_fem_gpiote_pin_task_subscribe_clear(&m_pa_gpiote_pin, false);
    }

    m_abort_event_is_set = false;

    return 0;
}

static void fem_psemi_lna_check_is_configured(int8_t *const p_gain)
{
	*p_gain = m_fem_interface_config.fem_config.lna_gain_db;
}
