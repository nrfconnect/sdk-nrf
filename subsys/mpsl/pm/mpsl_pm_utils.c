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

#include <mpsl/mpsl_work.h>
#include <mpsl/mpsl_pm_utils.h>

LOG_MODULE_REGISTER(mpsl_pm_utils, CONFIG_MPSL_LOG_LEVEL);

/* These constants must be updated once the Zephyr PM Policy API is updated
 * to handle low latency events. Ideally, the Policy API should be changed to use
 * absolute time instead of relative time. This would remove the need for safety
 * margins and allow optimal power savings.
 */
#define MAX_DELAY_SINCE_READING_PARAMS_US 50
#define TIME_TO_REGISTER_EVENT_IN_ZEPHYR_US 1000
#define PM_MAX_LATENCY_HCI_COMMANDS_US 4999999

static void m_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(pm_work, m_work_handler);

#define RETRY_TIME_MAX_US (UINT32_MAX - TIME_TO_REGISTER_EVENT_IN_ZEPHYR_US)

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

void mpsl_pm_utils_work_handler(void)
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
		/* No event scheduled, so set latency to restrict deepest sleep states*/
		m_update_latency_request(PM_MAX_LATENCY_HCI_COMMANDS_US);
		if (m_pm_event_is_registered) {
			pm_policy_event_unregister(&m_evt);
			m_pm_event_is_registered = false;
		}
		break;
	}
	case MPSL_PM_EVENT_STATE_BEFORE_EVENT:
	{
		/* Event scheduled */
		uint64_t event_time_us = params.event_time_rel_us -
						MAX_DELAY_SINCE_READING_PARAMS_US;

		/* In case we missed a state and are in zero-latency, set low-latency.*/
		m_update_latency_request(PM_MAX_LATENCY_HCI_COMMANDS_US);

		if (event_time_us > UINT32_MAX) {
			mpsl_work_schedule(&pm_work, K_USEC(RETRY_TIME_MAX_US));
			return;
		}

		if (m_pm_event_is_registered) {
			pm_policy_event_update(&m_evt, event_time_us);
		} else {
			pm_policy_event_register(&m_evt, event_time_us);
			m_pm_event_is_registered = true;
		}
		break;
	}
	case MPSL_PM_EVENT_STATE_IN_EVENT:
	{
		m_update_latency_request(0);
		break;
	}
	default:
		__ASSERT(false, "MPSL PM is in an undefined state.");
	}
	m_pm_prev_flag_value = params.cnt_flag;
}

static void m_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	mpsl_pm_utils_work_handler();
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
