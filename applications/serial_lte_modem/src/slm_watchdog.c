/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/watchdog.h>
#if defined(CONFIG_SLM_WATCHDOG)
#include "slm_watchdog.h"
#endif
#include <logging/log.h>
#include "slm_util.h"

LOG_MODULE_REGISTER(watchdog, CONFIG_SLM_LOG_LEVEL);

#define WDT_FEED_WORKER_DELAY_MS \
	((CONFIG_SLM_WATCHDOG_TIMEOUT_MSEC) / 2)

#define WDT_STACK_SIZE       KB(1)
#define WDT_PRIORITY      K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(wdt_stack_area, WDT_STACK_SIZE);

static struct k_work_q wdt_work_q;

struct wdt_data_storage {
	const struct device *wdt_drv;
	int wdt_channel_id;
	struct k_work_delayable workqueue_work;
};
static struct wdt_data_storage wdt_data;

static void primary_feed_worker(struct k_work *work_desc)
{
	int err = wdt_feed(wdt_data.wdt_drv, wdt_data.wdt_channel_id);

	LOG_DBG("Feeding watchdog");

	if (err) {
		LOG_ERR("Cannot feed watchdog. Error code: %d", err);
	} else {
		k_work_reschedule_for_queue(&wdt_work_q, &wdt_data.workqueue_work,
			K_MSEC(WDT_FEED_WORKER_DELAY_MS));
	}
}
static int watchdog_timeout_install(struct wdt_data_storage *data)
{
	static const struct wdt_timeout_cfg wdt_settings = {
		.window = {
			.min = 0,
			.max = CONFIG_SLM_WATCHDOG_TIMEOUT_MSEC,
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

	LOG_DBG("Watchdog timeout installed. Timeout: %d",
		CONFIG_SLM_WATCHDOG_TIMEOUT_MSEC);
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

	k_work_init_delayable(&data->workqueue_work, primary_feed_worker);

	int err = wdt_feed(data->wdt_drv, data->wdt_channel_id);

	if (err) {
		LOG_ERR("Cannot feed watchdog. Error code: %d", err);
		return err;
	}
	k_work_reschedule_for_queue(&wdt_work_q, &data->workqueue_work,
				K_MSEC(WDT_FEED_WORKER_DELAY_MS));

	if (err) {
		LOG_ERR("Cannot start watchdog feed worker!"
			" Error code: %d", err);
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

int slm_watchdog_init_and_start(void)
{
	/* start wdt wq */
	k_work_queue_start(&wdt_work_q, wdt_stack_area,
			   K_THREAD_STACK_SIZEOF(wdt_stack_area),
			   WDT_PRIORITY, NULL);
	return watchdog_enable(&wdt_data);
}
