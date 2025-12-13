/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <mpsl_pm.h>
#include <mpsl_pm_config.h>
#include <nrf_errno.h>
#include <zephyr/pm/policy.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE) || \
	defined(CONFIG_MPSL_PM_USE_MRAM_LATENCY_INTERNAL_SWITCH)
#include <mram_latency.h>
#endif

#include <mpsl/mpsl_work.h>
#include "mpsl_pm_utils.h"

LOG_MODULE_REGISTER(mpsl_pm_utils, CONFIG_MPSL_LOG_LEVEL);

#define NO_RADIO_EVENT_PERIOD_LATENCY_US CONFIG_MPSL_PM_NO_RADIO_EVENT_PERIOD_LATENCY_US

enum MPLS_PM_STATE {
	MPSL_PM_UNINITIALIZED,
	MPSL_PM_INITIALIZED
};

static uint8_t m_pm_prev_flag_value;
static bool m_pm_event_is_registered;
static uint32_t m_prev_lat_value_us;
static struct pm_policy_latency_request m_latency_req;
static struct pm_policy_event m_evt;

static atomic_t m_pm_state = (atomic_val_t)MPSL_PM_UNINITIALIZED;

#if defined(CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE)
#define LOW_LATENCY_ATOMIC_BITS_NUM 2
#define LOW_LATENCY_PM_BIT	    0
#define LOW_LATENCY_MRAM_BIT	    1
#define LOW_LATENCY_BITS_MASK	    0x3

static ATOMIC_DEFINE(m_low_latency_req_state, LOW_LATENCY_ATOMIC_BITS_NUM);
/* Variable must be global to use it in on-off service cancel or release API */
struct onoff_client m_mram_req_cli;

static void m_mram_low_latency_request(void);
static void m_mram_low_latency_release(void);
#endif /* CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE */

static void m_update_latency_request(uint32_t lat_value_us)
{
	if (m_prev_lat_value_us != lat_value_us) {
		pm_policy_latency_request_update(&m_latency_req, lat_value_us);
		m_prev_lat_value_us = lat_value_us;
	}
}

static void m_register_event(void)
{
	mpsl_pm_params_t params = {0};
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

#if defined(CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE)
			/* Request MRAM latency first because the call goes to system controller */
			m_mram_low_latency_request();
#endif /* CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE */

			m_update_latency_request(0);
#if defined(CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE)
			atomic_set_bit(m_low_latency_req_state, LOW_LATENCY_PM_BIT);

			/* Attempt to notify MPLS about change. Most likely it will happen later
			 * when MRAM low latency request is handled.
			 */
			if (atomic_test_bit(m_low_latency_req_state, LOW_LATENCY_MRAM_BIT)) {
#else
			if (true) {
#endif /* CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE*/
				mpsl_pm_low_latency_state_set(MPSL_PM_LOW_LATENCY_STATE_ON);
			}
		}
		break;
	case MPSL_PM_LOW_LATENCY_STATE_ON:
		if (!mpsl_pm_low_latency_requested()) {
			mpsl_pm_low_latency_state_set(MPSL_PM_LOW_LATENCY_STATE_RELEASING);

#if defined(CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE)
			m_mram_low_latency_release();
#endif /* CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE */
			m_update_latency_request(NO_RADIO_EVENT_PERIOD_LATENCY_US);
#if defined(CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE)
			atomic_clear_bit(m_low_latency_req_state, LOW_LATENCY_PM_BIT);
#endif /* CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE*/

			/* MRAM low release is handled sunchronously, hence the MPLS notification
			 * happens here.
			 */
			mpsl_pm_low_latency_state_set(MPSL_PM_LOW_LATENCY_STATE_OFF);
		}
		break;
	default:
		break;
	}
}

#if defined(CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE)
static void m_mram_request_cb(struct onoff_manager *mgr, struct onoff_client *cli, uint32_t state,
			      int res)
{
	if (res < 0) {
		/* Possible failure reasons:
		 *  # -ERRTIMEDOUT - nRFS service timeout
		 *  # -EIO - nRFS service error
		 *  # -ENXIO - request rejected
		 * All these mean failure for MPSL.
		 */
		__ASSERT(false, "MRAM low latency request could not be handled, reason: %d", res);
		return;
	}

	atomic_set_bit(m_low_latency_req_state, LOW_LATENCY_MRAM_BIT);

	if ((mpsl_pm_low_latency_state_get() == MPSL_PM_LOW_LATENCY_STATE_REQUESTING) &&
	    (atomic_test_bit(m_low_latency_req_state, LOW_LATENCY_PM_BIT))) {
		mpsl_pm_low_latency_state_set(MPSL_PM_LOW_LATENCY_STATE_ON);
	}
}

static void m_mram_low_latency_request(void)
{
	int err;

	sys_notify_init_callback(&m_mram_req_cli.notify, m_mram_request_cb);

	err = mram_no_latency_request(&m_mram_req_cli);

	if (err < 0) {
		__ASSERT(false, "MPSL MRAM low latency request failed, err: %d\n", err);
		return;
	}
}

static void m_mram_low_latency_release(void)
{
	int err;

	err = mram_no_latency_cancel_or_release(&m_mram_req_cli);
	if (err < 0) {
		__ASSERT(false, "MPSL MRAM low latency release failed, err: %d\n", err);
		return;
	}

	/* The mram_no_latency_cancel_or_release() is sunchronous. There is no ansynchronous way to
	 * release an MRAM low latency request, due for lack of such support in on-off Zephyr's
	 * service.
	 */
	atomic_clear_bit(m_low_latency_req_state, LOW_LATENCY_MRAM_BIT);
}
#endif /* CONFIG_MPSL_PM_USE_MRAM_LATENCY_SERVICE */

void mpsl_pm_utils_work_handler(void)
{
	enum MPLS_PM_STATE pm_state = (enum MPLS_PM_STATE)atomic_get(&m_pm_state);

	if (pm_state == MPSL_PM_INITIALIZED) {
		m_register_event();
		m_register_latency();
	}
}

int32_t mpsl_pm_utils_init(void)
{
	mpsl_pm_params_t params = {0};

	if (atomic_get(&m_pm_state) != (atomic_val_t)MPSL_PM_UNINITIALIZED) {
		return -NRF_EPERM;
	}

	pm_policy_latency_request_add(&m_latency_req, NO_RADIO_EVENT_PERIOD_LATENCY_US);
	m_prev_lat_value_us = NO_RADIO_EVENT_PERIOD_LATENCY_US;

#if defined(CONFIG_MPSL_PM_USE_MRAM_LATENCY_INTERNAL_SWITCH)
	int err;

	err = mram_no_latency_internal_switch_enable();
	__ASSERT(0 == err, "MRAM internal switch enable failed: %d.", err);
#endif /* CONFIG_MPSL_PM_USE_MRAM_LATENCY_INTERNAL_SWITCH */

	mpsl_pm_init();
	/* On init there should be no update from high-prio, returned value can be ignored */
	(void)mpsl_pm_params_get(&params);
	m_pm_prev_flag_value = params.cnt_flag;
	m_pm_event_is_registered = false;

	atomic_set(&m_pm_state, (atomic_val_t)MPSL_PM_INITIALIZED);

	return 0;
}

int32_t mpsl_pm_utils_uninit(void)
{
	if (atomic_get(&m_pm_state) != (atomic_val_t)MPSL_PM_INITIALIZED) {
		return -NRF_EPERM;
	}

	mpsl_pm_uninit();

#if defined(CONFIG_MPSL_PM_USE_MRAM_LATENCY_INTERNAL_SWITCH)
	int err;

	err = mram_no_latency_internal_switch_disable();
	__ASSERT(0 == err, "MRAM internal switch disable failed: %d.", err);
#endif /* CONFIG_MPSL_PM_USE_MRAM_LATENCY_INTERNAL_SWITCH */

	/* The mpsl_pm_utils_uninit() must be called with MULTITHREADING_LOCK_ACQUIRE to make sure
	 * there is no mpsl_low_prio_work running. That could lead to a race condition.
	 * Call to MULTITHREADING_LOCK_ACQUIRE() is responsibility of the function caller.
	 */
	pm_policy_latency_request_remove(&m_latency_req);
	pm_policy_event_unregister(&m_evt);

	atomic_set(&m_pm_state, (atomic_val_t)MPSL_PM_UNINITIALIZED);

	return 0;
}
