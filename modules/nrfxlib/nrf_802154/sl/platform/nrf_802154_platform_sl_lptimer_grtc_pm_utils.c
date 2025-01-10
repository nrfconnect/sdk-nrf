/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_802154_platform_sl_lptimer_grtc_pm_utils.h"

#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>

#define LPTIMER_PM_UTILS_STACK_SIZE 256

static K_SEM_DEFINE(m_lptimer_pm_utils_sem, 0, 1);

static struct pm_policy_event m_pm_event;
static int64_t                m_trigger_time;
static bool		      m_event_registered;

#define GRTC_CYC_PER_SYS_TICK \
	((uint64_t)sys_clock_hw_cycles_per_sec() / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)

static int64_t grtc_ticks_to_system_ticks(uint64_t abs_grtc)
{
	int64_t curr_sys = k_uptime_ticks();
	uint64_t curr_grtc = z_nrf_grtc_timer_read();

	uint64_t result = (abs_grtc > curr_grtc) ? (abs_grtc - curr_grtc) : 0;

	return curr_sys + result / GRTC_CYC_PER_SYS_TICK;
}

void nrf_802154_platform_sl_lptimer_pm_utils_event_update(uint64_t grtc_ticks)
{
	int64_t event_time = (grtc_ticks != 0) ? grtc_ticks_to_system_ticks(grtc_ticks) : 0;

	if (m_trigger_time != event_time) {
		m_trigger_time = event_time;

		k_sem_give(&m_lptimer_pm_utils_sem);
	}
}

static void lptimer_pm_utils_thread_func(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	while (true) {
		(void)k_sem_take(&m_lptimer_pm_utils_sem, K_FOREVER);

		if (m_trigger_time == 0) {
			if (m_event_registered) {
				pm_policy_event_unregister(&m_pm_event);
			}
			m_event_registered = false;
		} else if (!m_event_registered) {
			pm_policy_event_register(&m_pm_event, m_trigger_time);
			m_event_registered = true;
		} else {
			pm_policy_event_update(&m_pm_event, m_trigger_time);
		}
	}
}

K_THREAD_DEFINE(lptimer_pm_utils_thread, LPTIMER_PM_UTILS_STACK_SIZE,
	lptimer_pm_utils_thread_func, NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
