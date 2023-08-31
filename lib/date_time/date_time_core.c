/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <date_time.h>
#include <zephyr/posix/time.h>
#include <zephyr/logging/log.h>
#include <modem/at_monitor.h>
#include <modem/lte_lc.h>

#include "date_time_core.h"
#include "date_time_modem.h"
#include "date_time_ntp.h"

LOG_MODULE_DECLARE(date_time, CONFIG_DATE_TIME_LOG_LEVEL);

BUILD_ASSERT(CONFIG_DATE_TIME_TOO_OLD_SECONDS <= CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS);

#define DATE_TIME_EVT_TYPE_PREVIOUS 0xFF

static K_SEM_DEFINE(time_fetch_sem, 0, 1);

static void date_time_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(time_work, date_time_handler);

static int64_t date_time_last_update_uptime;
static date_time_evt_handler_t app_evt_handler;

static void date_time_core_notify_event(enum date_time_evt_type time_source)
{
	static struct date_time_evt evt;

	/* Update time source to the event if not requesting previous time source */
	if (time_source != DATE_TIME_EVT_TYPE_PREVIOUS) {
		evt.type = time_source;
	}

	if (app_evt_handler != NULL) {
		app_evt_handler(&evt);
	}
}

static void date_time_core_schedule_update(bool check_pending)
{
	/* (Re-)schedule time update work
	 * if periodic updates are requested in the configuration.
	 */
	if (CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS > 0) {
		/* Don't reschedule time update work in some cases,
		 * e.g. if time is not obtained and the work is already pending.
		 */
		if (check_pending && k_work_delayable_is_pending(&time_work)) {
			return;
		}

		LOG_DBG("New periodic date time update in: %d seconds",
			CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS);

		k_work_reschedule(&time_work, K_SECONDS(CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS));
	}
}

static void date_time_update_thread(void)
{
	int err;

	while (true) {
		k_sem_take(&time_fetch_sem, K_FOREVER);

		LOG_DBG("Updating date time UTC...");

		err = date_time_core_current_check();
		if (err == 0) {
			LOG_DBG("Using previously obtained time");
			date_time_core_schedule_update(true);
			date_time_core_notify_event(DATE_TIME_EVT_TYPE_PREVIOUS);
			continue;
		}

#if defined(CONFIG_DATE_TIME_MODEM)
		LOG_DBG("Getting time from cellular network");
		int64_t date_time_ms_modem = 0;

		err = date_time_modem_get(&date_time_ms_modem);
		if (err == 0) {
			date_time_core_store(date_time_ms_modem, DATE_TIME_OBTAINED_MODEM);
			continue;
		}
#endif
#if defined(CONFIG_DATE_TIME_NTP)
		LOG_DBG("Getting time from NTP server");
		int64_t date_time_ms_ntp = 0;

		err = date_time_ntp_get(&date_time_ms_ntp);
		if (err == 0) {
			date_time_core_store(date_time_ms_ntp, DATE_TIME_OBTAINED_NTP);
			continue;
		}
#endif
		LOG_DBG("Did not get time from any time source");

		date_time_core_schedule_update(true);
		date_time_core_notify_event(DATE_TIME_NOT_OBTAINED);
	}
}

K_THREAD_DEFINE(time_thread, CONFIG_DATE_TIME_THREAD_STACK_SIZE,
		date_time_update_thread, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

static void date_time_handler(struct k_work *work)
{
	k_sem_give(&time_fetch_sem);
}

void date_time_lte_ind_handler(const struct lte_lc_evt *const evt)
{
#if defined(CONFIG_DATE_TIME_AUTO_UPDATE) && defined(CONFIG_LTE_LINK_CONTROL)
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:

		switch (evt->nw_reg_status) {
		case LTE_LC_NW_REG_REGISTERED_HOME:
		case LTE_LC_NW_REG_REGISTERED_ROAMING:
			if (!date_time_is_valid()) {
				k_work_reschedule(&time_work, K_SECONDS(1));
			}
			break;
#if defined(CONFIG_DATE_TIME_MODEM)
		case LTE_LC_NW_REG_SEARCHING:
			date_time_modem_xtime_subscribe();
			break;
#endif
		default:
			break;
		}
		break;
	default:
		break;
	}
#endif /* defined(CONFIG_DATE_TIME_AUTO_UPDATE) && defined(CONFIG_LTE_LINK_CONTROL) */
}

void date_time_core_init(void)
{
	if (IS_ENABLED(CONFIG_DATE_TIME_AUTO_UPDATE) && IS_ENABLED(CONFIG_LTE_LINK_CONTROL)) {
		lte_lc_register_handler(date_time_lte_ind_handler);
	}

	if (!IS_ENABLED(CONFIG_DATE_TIME_AUTO_UPDATE)) {
		date_time_core_schedule_update(false);
	}
}

int date_time_core_now(int64_t *unix_time_ms)
{
	int err;
	struct timespec tp;

	err = clock_gettime(CLOCK_REALTIME, &tp);
	if (err) {
		LOG_WRN("clock_gettime failed, errno %d", errno);
		return -ENODATA;
	}
	*unix_time_ms = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;

	return 0;
}

int date_time_core_update_async(date_time_evt_handler_t evt_handler)
{
	if (evt_handler) {
		app_evt_handler = evt_handler;
	} else if (app_evt_handler == NULL) {
		LOG_DBG("No handler registered");
	}

	k_sem_give(&time_fetch_sem);

	return 0;
}

void date_time_core_register_handler(date_time_evt_handler_t evt_handler)
{
	if (evt_handler == NULL) {
		app_evt_handler = NULL;

		LOG_DBG("Previously registered handler %p de-registered",
			app_evt_handler);

		return;
	}

	LOG_DBG("Registering handler %p", evt_handler);

	app_evt_handler = evt_handler;
}

bool date_time_core_is_valid(void)
{
	return (date_time_last_update_uptime != 0);
}

void date_time_core_clear(void)
{
	date_time_last_update_uptime = 0;
}

int date_time_core_current_check(void)
{
	if (date_time_last_update_uptime == 0) {
		LOG_DBG("Date time never set");
		return -ENODATA;
	}

	if ((k_uptime_get() - date_time_last_update_uptime) >=
	    CONFIG_DATE_TIME_TOO_OLD_SECONDS * MSEC_PER_SEC) {
		LOG_DBG("Current date time too old");
		return -ENODATA;
	}

	return 0;
}

void date_time_core_store(int64_t curr_time_ms, enum date_time_evt_type time_source)
{
	struct timespec tp = { 0 };
	struct tm ltm = { 0 };
	int ret;

	date_time_last_update_uptime = k_uptime_get();

	date_time_core_schedule_update(false);

	tp.tv_sec = curr_time_ms / 1000;
	tp.tv_nsec = (curr_time_ms % 1000) * 1000000;

	ret = clock_settime(CLOCK_REALTIME, &tp);
	if (ret != 0) {
		LOG_ERR("Could not set system time, %d", ret);
		date_time_core_notify_event(DATE_TIME_NOT_OBTAINED);
		return;
	}
	gmtime_r(&tp.tv_sec, &ltm);
	LOG_DBG("System time updated: %04u-%02u-%02u %02u:%02u:%02u",
		ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
		ltm.tm_hour, ltm.tm_min, ltm.tm_sec);

#if defined(CONFIG_DATE_TIME_MODEM)
	date_time_modem_store(&ltm);
#endif

	date_time_core_notify_event(time_source);
}
