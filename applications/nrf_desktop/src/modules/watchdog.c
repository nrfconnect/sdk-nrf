/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>

#define MODULE watchdog
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_WATCHDOG_LOG_LEVEL);

#define WDT_FEED_WORKER_DELAY_MS ((CONFIG_DESKTOP_WATCHDOG_TIMEOUT)/3)

struct wdt_data_storage {
	const struct device *const wdt;
	int wdt_channel_id;
	struct k_work_delayable work;
};

static struct wdt_data_storage wdt_data = {
	.wdt = DEVICE_DT_GET(DT_NODELABEL(wdt)),
};

static void watchdog_feed_worker(struct k_work *work_desc)
{
	struct wdt_data_storage *data =
			CONTAINER_OF(work_desc, struct wdt_data_storage, work);

	int err = wdt_feed(data->wdt, data->wdt_channel_id);

	if (err) {
		LOG_ERR("Cannot feed watchdog. Error code: %d", err);
		module_set_state(MODULE_STATE_ERROR);
	} else {
		k_work_reschedule(&data->work,
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
			data->wdt, &wdt_settings);
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

	int err = wdt_setup(data->wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);

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

	k_work_init_delayable(&data->work, watchdog_feed_worker);
	int ret = k_work_schedule(&data->work, K_NO_WAIT);

	if (ret != 1) {
		LOG_ERR("Cannot start watchdog feed worker! Error code: %d", ret);
		ret = (ret == 0) ? (-EALREADY) : (ret);
	} else {
		LOG_INF("Watchdog feed enabled. Timeout: %d", WDT_FEED_WORKER_DELAY_MS);
		ret = 0;
	}

	return ret;
}

static int watchdog_enable(struct wdt_data_storage *data)
{
	__ASSERT_NO_MSG(data != NULL);

	int err;

	if (!device_is_ready(data->wdt)) {
		LOG_ERR("Watchdog device not ready");
		return -ENODEV;
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

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
				cast_module_state_event(aeh);

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

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, module_state_event);
