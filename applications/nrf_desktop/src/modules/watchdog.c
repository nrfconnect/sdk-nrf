/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/watchdog.h>

#define MODULE watchdog
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_WATCHDOG_LOG_LEVEL);

#define WDT_FEED_WORKER_DELAY_MS ((CONFIG_DESKTOP_WATCHDOG_TIMEOUT)/3)

struct wdt_data_storage {
	struct device *wdt_drv;
	int wdt_channel_id;
	struct k_delayed_work work;
};
static struct wdt_data_storage wdt_data;

static void watchdog_feed_worker(struct k_work *work_desc)
{
	struct wdt_data_storage *data =
			CONTAINER_OF(work_desc, struct wdt_data_storage, work);

	int err = wdt_feed(data->wdt_drv, data->wdt_channel_id);

	if (err) {
		LOG_ERR("Cannot feed watchdog. Error code: %d", err);
		module_set_state(MODULE_STATE_ERROR);
	} else {
		k_delayed_work_submit(&data->work,
				      K_MSEC(WDT_FEED_WORKER_DELAY_MS));
	}
}

static int watchdog_timeout_install(struct wdt_data_storage *data)
{
	static const struct wdt_timeout_cfg wdt_settings = {
			.window = {
				.min = 0,
				.max = CONFIG_DESKTOP_WATCHDOG_TIMEOUT
			},
			.callback = NULL,
			.flags = WDT_FLAG_RESET_SOC
	};

	__ASSERT_NO_MSG(data != NULL);

	data->wdt_channel_id = wdt_install_timeout(
			data->wdt_drv, &wdt_settings);
	if (data->wdt_channel_id < 0) {
		LOG_ERR("Cannot install watchdog timer! Error code: %d",
			data->wdt_channel_id);
		return -EFAULT;
	}

	LOG_INF("Watchdog timeout installed. Timeout: %d", CONFIG_DESKTOP_WATCHDOG_TIMEOUT);
	return 0;
}

static int watchdog_start(struct wdt_data_storage *data)
{
	__ASSERT_NO_MSG(data != NULL);

	int err = wdt_setup(data->wdt_drv, WDT_OPT_PAUSE_HALTED_BY_DBG);

	if (err) {
		LOG_ERR("Cannot start watchdog! Error code: %d", err);
	} else {
		LOG_INF("Watchdog started");
	}
	return err;
}

static int watchdog_feed_enable(struct wdt_data_storage *data)
{
	__ASSERT_NO_MSG(data != NULL);

	k_delayed_work_init(&data->work, watchdog_feed_worker);
	int err = k_delayed_work_submit(&data->work, K_NO_WAIT);

	if (err) {
		LOG_ERR("Cannot start watchdog feed worker!"
				" Error code: %d", err);
	} else {
		LOG_INF("Watchdog feed enabled. Timeout: %d", WDT_FEED_WORKER_DELAY_MS);
	}
	return err;
}

static int watchdog_enable(struct wdt_data_storage *data)
{
	__ASSERT_NO_MSG(data != NULL);

	int err = -ENXIO;

	data->wdt_drv = device_get_binding(DT_LABEL(DT_NODELABEL(wdt)));
	if (data->wdt_drv == NULL) {
		LOG_ERR("Cannot bind watchdog driver");
		return err;
	}

	err = watchdog_timeout_install(data);

	if (err) {
		return err;
	}

	err = watchdog_feed_enable(data);
	if (err) {
		return err;
	}

	err = watchdog_start(data);
	if (err) {
		return err;
	}
	return 0;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
				cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (watchdog_enable(&wdt_data)) {
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, module_state_event);
