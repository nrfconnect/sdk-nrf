/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* Required for gmtime_r */

#include <date_time.h>
#include <zephyr/logging/log.h>
#include <modem/at_monitor.h>
#include <modem/lte_lc.h>

#include "date_time_core.h"
#include "date_time_modem.h"
#include "date_time_ntp.h"

#if defined(CONFIG_ARCH_POSIX) && defined(CONFIG_EXTERNAL_LIBC)
#include <time.h>
#else
#include <zephyr/posix/time.h>
#endif

LOG_MODULE_DECLARE(date_time, CONFIG_DATE_TIME_LOG_LEVEL);

BUILD_ASSERT(CONFIG_DATE_TIME_TOO_OLD_SECONDS <= CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS);

#define DATE_TIME_EVT_TYPE_PREVIOUS 0xFF

K_THREAD_STACK_DEFINE(date_time_stack, CONFIG_DATE_TIME_THREAD_STACK_SIZE);
struct k_work_q date_time_work_q;

static void date_time_update_work_fn(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(date_time_update_work, date_time_update_work_fn);

static K_WORK_DEFINE(date_time_update_manual_work, date_time_update_work_fn);

static int64_t date_time_last_update_uptime;
static date_time_evt_handler_t app_evt_handler;

/* In units of quarters of hours, same as used by AT+CCLK. */
static int date_time_tz = DATE_TIME_TZ_INVALID;

/* Whether or not an already-scheduled update is blocking reschedules. */
static atomic_t reschedule_blocked;

/* Number of consecutive retries that have been attempted so far */
static atomic_t retry_count;

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

static int date_time_core_schedule_work(int interval)
{
	/* If a scheduled update is blocking reschedules, exit.
	 * Otherwise set the reschedule_blocked flag to true, then proceed with the reschedule.
	 */

	if (atomic_set(&reschedule_blocked, true)) {
		LOG_DBG("Requested date-time update not scheduled, another is already scheduled");
		return -EAGAIN;
	}

	k_work_reschedule_for_queue(&date_time_work_q, &date_time_update_work, K_SECONDS(interval));

	return 0;
}

static void date_time_core_schedule_update(void)
{
	if (CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS <= 0) {
		LOG_DBG("Skipping requested date time update, periodic requests are not enabled");
		return;
	}

	/* Reset the retry counter since this will be a normal update. */
	atomic_set(&retry_count, 0);

	if (date_time_core_schedule_work(CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS) == 0) {
		LOG_DBG("New periodic date time update in: %d seconds",
			CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS);
	}
}

static void date_time_core_schedule_retry(void)
{
	/* If this is the last allowable retry, or retries are not enabled
	 * (CONFIG_DATE_TIME_RETRY_COUNT==0), switch to performing a normal update.
	 */
	if (atomic_inc(&retry_count) >= CONFIG_DATE_TIME_RETRY_COUNT) {
		date_time_core_schedule_update();
		return;
	}

	if (date_time_core_schedule_work(CONFIG_DATE_TIME_RETRY_INTERVAL_SECONDS) == 0) {
		LOG_DBG("Date time update retry in: %d seconds",
			CONFIG_DATE_TIME_RETRY_INTERVAL_SECONDS);
	}
}

static void date_time_update_work_fn(struct k_work *work)
{
	int err;

	LOG_DBG("Updating date time UTC...");

	err = date_time_core_current_check();
	if (err == 0) {
		LOG_DBG("Using previously obtained time");
		date_time_core_schedule_update();
		date_time_core_notify_event(DATE_TIME_EVT_TYPE_PREVIOUS);
		return;
	}

	/* Unblock update reschedules since we are now performing an update.
	 * There may still be a scheduled update beyond this one (for example if this was invoked
	 * by date_time_core_update_async), but it will be overridden when this update triggers a
	 * reschedule.
	 */
	atomic_clear(&reschedule_blocked);

#if defined(CONFIG_DATE_TIME_MODEM)
	LOG_DBG("Getting time from cellular network");
	int tz = DATE_TIME_TZ_INVALID;
	int64_t date_time_ms_modem = 0;

	err = date_time_modem_get(&date_time_ms_modem, &tz);
	if (err == 0) {
		date_time_core_store_tz(date_time_ms_modem, DATE_TIME_OBTAINED_MODEM, tz);
		return;
	}
#endif
#if defined(CONFIG_DATE_TIME_NTP)
	LOG_DBG("Getting time from NTP server");
	int64_t date_time_ms_ntp = 0;

	err = date_time_ntp_get(&date_time_ms_ntp);
	if (err == 0) {
		date_time_core_store(date_time_ms_ntp, DATE_TIME_OBTAINED_NTP);
		return;
	}
#endif
	LOG_DBG("Did not get time from any time source");

	/* If CONFIG_DATE_TIME_RETRY_COUNT is set to 0, this will turn into a normal update. */
	date_time_core_schedule_retry();
	date_time_core_notify_event(DATE_TIME_NOT_OBTAINED);
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
				k_work_reschedule_for_queue(
					&date_time_work_q,
					&date_time_update_work,
					K_SECONDS(1));
			}
			break;
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
	struct k_work_queue_config cfg = {
		.name = "date_time_work_q",
	};

	k_work_queue_start(
		&date_time_work_q,
		date_time_stack,
		CONFIG_DATE_TIME_THREAD_STACK_SIZE,
		K_LOWEST_APPLICATION_THREAD_PRIO,
		&cfg);

	if (IS_ENABLED(CONFIG_DATE_TIME_AUTO_UPDATE) && IS_ENABLED(CONFIG_LTE_LINK_CONTROL)) {
		lte_lc_register_handler(date_time_lte_ind_handler);
	}

	if (!IS_ENABLED(CONFIG_DATE_TIME_AUTO_UPDATE)) {
		date_time_core_schedule_update();
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

int date_time_core_now_local(int64_t *local_time_ms)
{
	int err;

	if (date_time_tz == DATE_TIME_TZ_INVALID) {
		return -EAGAIN;
	}

	err = date_time_core_now(local_time_ms);
	if (err) {
		return err;
	}

	*local_time_ms += date_time_tz * 15 * 60 * 1000;
	return 0;
}

int date_time_core_update_async(date_time_evt_handler_t evt_handler)
{
	if (evt_handler) {
		app_evt_handler = evt_handler;
	} else if (app_evt_handler == NULL) {
		LOG_DBG("No handler registered");
	}

	/* Cannot reschedule date_time_update_work because it would mess up normal update cycle */
	k_work_submit_to_queue(&date_time_work_q, &date_time_update_manual_work);

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

bool date_time_core_is_valid_local(void)
{
	if (date_time_tz == DATE_TIME_TZ_INVALID) {
		return false;
	}

	return date_time_core_is_valid();
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
	/* Preserve existing TZ */
	date_time_core_store_tz(curr_time_ms, time_source, date_time_tz);
}

void date_time_core_store_tz(int64_t curr_time_ms, enum date_time_evt_type time_source, int tz)
{
	struct timespec tp = {0};
	struct tm ltm = {0};
	int ret;

	date_time_last_update_uptime = k_uptime_get();

	date_time_tz = tz;

	date_time_core_schedule_update();

	/* Reset the retry counter since we have successfully acquired a time. */
	atomic_set(&retry_count, 0);

	tp.tv_sec = curr_time_ms / 1000;
	tp.tv_nsec = (curr_time_ms % 1000) * 1000000;

	ret = clock_settime(CLOCK_REALTIME, &tp);
	if (ret != 0) {
		LOG_ERR("Could not set system time, %d", ret);
		date_time_core_notify_event(DATE_TIME_NOT_OBTAINED);
		return;
	}
	gmtime_r(&tp.tv_sec, &ltm);
	LOG_DBG("System time updated: %04u-%02u-%02u %02u:%02u:%02u%+03d",
		ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
		ltm.tm_hour, ltm.tm_min, ltm.tm_sec, tz);

#if defined(CONFIG_DATE_TIME_MODEM)
	if (time_source != DATE_TIME_OBTAINED_MODEM) {
		date_time_modem_store(&ltm, tz);
	}
#endif

	date_time_core_notify_event(time_source);
}
