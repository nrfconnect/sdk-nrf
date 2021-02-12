/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/watchdog.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(watchdog, CONFIG_WATCHDOG_LOG_LEVEL);

#define WDT_FEED_WORKER_DELAY_MS					\
	((CONFIG_WATCHDOG_APPLICATION_TIMEOUT_SEC * 1000) / 2)
#define WATCHDOG_TIMEOUT_MSEC						\
	(CONFIG_WATCHDOG_APPLICATION_TIMEOUT_SEC * 1000)

struct wdt_data_storage {
	const struct device *wdt_drv;
	int wdt_channel_id;
	struct k_delayed_work system_workqueue_work;
};

static struct wdt_data_storage wdt_data;

static void primary_feed_worker(struct k_work *work_desc)
{
	int err = wdt_feed(wdt_data.wdt_drv, wdt_data.wdt_channel_id);

	LOG_DBG("Feeding watchdog");

	if (err) {
		LOG_ERR("Cannot feed watchdog. Error code: %d", err);
	} else {
		k_delayed_work_submit(&wdt_data.system_workqueue_work,
				      K_MSEC(WDT_FEED_WORKER_DELAY_MS));
	}
}

static int watchdog_timeout_install(struct wdt_data_storage *data)
{
	static const struct wdt_timeout_cfg wdt_settings = {
		.window = {
			.min = 0,
			.max = WATCHDOG_TIMEOUT_MSEC,
		},
		.callback = NULL,
		.flags = WDT_FLAG_RESET_SOC
	};

	__ASSERT_NO_MSG(data != NULL);

	data->wdt_channel_id =
		wdt_install_timeout(data->wdt_drv, &wdt_settings);
	if (data->wdt_channel_id < 0) {
		LOG_ERR("Cannot install watchdog timer! Error code: %d",
			data->wdt_channel_id);
		return -EFAULT;
	}

	LOG_DBG("Watchdog timeout installed. Timeout: %d",
		CONFIG_WATCHDOG_APPLICATION_TIMEOUT_SEC);
	return 0;
}

static int watchdog_start(struct wdt_data_storage *data)
{
	__ASSERT_NO_MSG(data != NULL);

	int err = wdt_setup(data->wdt_drv, WDT_OPT_PAUSE_HALTED_BY_DBG);

	if (err) {
		LOG_ERR("Cannot start watchdog! Error code: %d", err);
	} else {
		LOG_DBG("Watchdog started");
	}
	return err;
}

static int watchdog_feed_enable(struct wdt_data_storage *data)
{
	__ASSERT_NO_MSG(data != NULL);

	k_delayed_work_init(&data->system_workqueue_work, primary_feed_worker);

	int err = wdt_feed(data->wdt_drv, data->wdt_channel_id);

	if (err) {
		LOG_ERR("Cannot feed watchdog. Error code: %d", err);
		return err;
	}

	err = k_delayed_work_submit(&data->system_workqueue_work,
				    K_MSEC(WDT_FEED_WORKER_DELAY_MS));
	if (err) {
		LOG_ERR("Cannot start watchdog feed worker!"
			" Error code: %d",
			err);
	} else {
		LOG_DBG("Watchdog feed enabled. Timeout: %d",
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

int watchdog_init_and_start(void)
{
	return watchdog_enable(&wdt_data);
}
