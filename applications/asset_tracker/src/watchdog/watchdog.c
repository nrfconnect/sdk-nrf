/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/watchdog.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(watchdog, CONFIG_ASSET_TRACKER_LOG_LEVEL);

#define WDT_FEED_WORKER_DELAY_MS \
	((CONFIG_ASSET_TRACKER_WATCHDOG_TIMEOUT_MSEC)/2)

struct wdt_data_storage {
	struct device *wdt_drv;
	int wdt_channel_id;
	struct k_delayed_work system_workqueue_work;
	struct k_work second_workqueue_work;
};
static struct wdt_data_storage wdt_data;
static struct k_work_q *second_work_q;

static void primary_feed_worker(struct k_work *work_desc)
{
	k_work_submit_to_queue(second_work_q, &wdt_data.second_workqueue_work);
}
static void secondary_feed_worker(struct k_work *work_desc)
{
	int err = wdt_feed(wdt_data.wdt_drv, wdt_data.wdt_channel_id);

	LOG_DBG("Feeding watchdog");

	if (err) {
		LOG_ERR("Cannot feed watchdog. Error code: %d", err);
	} else {
		k_delayed_work_submit(&wdt_data.system_workqueue_work,
				      WDT_FEED_WORKER_DELAY_MS);
	}
}

static int watchdog_timeout_install(struct wdt_data_storage *data)
{
	static const struct wdt_timeout_cfg wdt_settings = {
			.window = {
				.min = 0,
				.max = CONFIG_ASSET_TRACKER_WATCHDOG_TIMEOUT_MSEC,
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

	LOG_INF("Watchdog timeout installed. Timeout: %d",
		CONFIG_ASSET_TRACKER_WATCHDOG_TIMEOUT_MSEC);
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

	k_delayed_work_init(&data->system_workqueue_work, primary_feed_worker);
	k_work_init(&data->second_workqueue_work, secondary_feed_worker);

	int err = wdt_feed(data->wdt_drv, data->wdt_channel_id);

	if (err) {
		LOG_ERR("Cannot feed watchdog. Error code: %d", err);
		return err;
	}

	err = k_delayed_work_submit(&data->system_workqueue_work,
				    WDT_FEED_WORKER_DELAY_MS);
	if (err) {
		LOG_ERR("Cannot start watchdog feed worker!"
				" Error code: %d", err);
	} else {
		LOG_INF("Watchdog feed enabled. Timeout: %d",
			WDT_FEED_WORKER_DELAY_MS);
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

	err = watchdog_start(data);
	if (err) {
		return err;
	}

	err = watchdog_feed_enable(data);
	if (err) {
		return err;
	}

	return 0;
}

int watchdog_init_and_start(struct k_work_q *work_q)
{
	if (work_q == NULL) {
		return -EINVAL;
	}
	second_work_q = work_q;
	return watchdog_enable(&wdt_data);
}
