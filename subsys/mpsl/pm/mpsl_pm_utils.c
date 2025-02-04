/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <mpsl_pm.h>
#include <mpsl_pm_config.h>
#include <zephyr/pm/policy.h>
#include <zephyr/logging/log.h>

#include <mpsl/mpsl_pm_utils.h>

LOG_MODULE_REGISTER(mpsl_pm_utils, CONFIG_MPSL_LOG_LEVEL);

/* These constants must be updated once the Zephyr PM Policy API is updated
 * to handle low latency events. Ideally, the Policy API should be changed to use
 * absolute time instead of relative time. This would remove the need for safety
 * margins and allow optimal power savings.
 */
#define TIME_TO_REGISTER_EVENT_IN_ZEPHYR_US 1000
#define PM_MAX_LATENCY_HCI_COMMANDS_US 499999

static uint8_t                          m_pm_prev_flag_value;
static bool                             m_pm_event_is_registered;
static uint32_t                         m_prev_lat_value_us;
static struct pm_policy_latency_request m_latency_req;
static struct pm_policy_event           m_evt;


static void m_update_latency_request(uint32_t lat_value_us)
{
	if (m_prev_lat_value_us != lat_value_us) {
		pm_policy_latency_request_update(&m_latency_req, lat_value_us);
		m_prev_lat_value_us = lat_value_us;
	}
}

void m_register_event(void)
{
	mpsl_pm_params_t params	= {0};
	bool pm_param_valid = mpsl_pm_params_get(&params);

	if (m_pm_prev_flag_value == params.cnt_flag) {
		/* We have no new info to process.*/
		return;
	}
	if (!pm_param_valid) {
		/* High prio did change mpsl_pm_params, while we read the params. */
		m_pm_prev_flag_value = params.cnt_flag;
		return;
	}
	switch (params.event_state) {
	case MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT:
	{
		if (m_pm_event_is_registered) {
			pm_policy_event_unregister(&m_evt);
			m_pm_event_is_registered = false;
		}
		break;
	}
	case MPSL_PM_EVENT_STATE_BEFORE_EVENT:
	{
		/* Event scheduled */
		if (m_pm_event_is_registered) {
			pm_policy_event_update(&m_evt,
					       k_us_to_ticks_floor64(params.event_time_abs_us));
		} else {
			pm_policy_event_register(&m_evt,
						 k_us_to_ticks_floor64(params.event_time_abs_us));
			m_pm_event_is_registered = true;
		}
		break;
	}
	default:
		__ASSERT(false, "MPSL PM is in an undefined state.");
	}
	m_pm_prev_flag_value = params.cnt_flag;
}

static void m_register_latency(void)
{
	switch (mpsl_pm_low_latency_state_get()) {
	case MPSL_PM_LOW_LATENCY_STATE_OFF:
		if (mpsl_pm_low_latency_requested()) {
			mpsl_pm_low_latency_state_set(MPSL_PM_LOW_LATENCY_STATE_REQUESTING);
			m_update_latency_request(0);
			mpsl_pm_low_latency_state_set(MPSL_PM_LOW_LATENCY_STATE_ON);
		}
		break;
	case MPSL_PM_LOW_LATENCY_STATE_ON:
		if (!mpsl_pm_low_latency_requested()) {
			mpsl_pm_low_latency_state_set(MPSL_PM_LOW_LATENCY_STATE_RELEASING);
			m_update_latency_request(PM_MAX_LATENCY_HCI_COMMANDS_US);
			mpsl_pm_low_latency_state_set(MPSL_PM_LOW_LATENCY_STATE_OFF);
		}
		break;
	default:
		break;
	}
}

void mpsl_pm_utils_work_handler(void)
{
	m_register_event();
	m_register_latency();
}

void mpsl_pm_utils_init(void)
{
	mpsl_pm_params_t params = {0};

	pm_policy_latency_request_add(&m_latency_req, PM_MAX_LATENCY_HCI_COMMANDS_US);
	m_prev_lat_value_us = PM_MAX_LATENCY_HCI_COMMANDS_US;

	mpsl_pm_init();
	mpsl_pm_params_get(&params);
	m_pm_prev_flag_value = params.cnt_flag;
	m_pm_event_is_registered = false;
}
