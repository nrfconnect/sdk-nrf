/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt, LOG_LEVEL_INF);

#include <zephyr/drivers/watchdog.h>
#include "common.h"


static const struct device *const my_wdt_device = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static struct wdt_timeout_cfg m_cfg_wdt0;
static int my_wdt_channel;

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
#define NOINIT_SECTION ".dtcm_noinit.test_wdt"
#else
#define NOINIT_SECTION ".noinit.test_wdt"
#endif
volatile uint32_t wdt_status __attribute__((section(NOINIT_SECTION)));

static void wdt_int_cb(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	wdt_status = WDT_TAG;
}


/* WDT thread */
static void wdt_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;

	atomic_inc(&started_threads);

	if (!device_is_ready(my_wdt_device)) {
		LOG_ERR("WDT device %s is not ready", my_wdt_device->name);
		atomic_inc(&completed_threads);
		return;
	}

	/* Configure Watchdog */
	m_cfg_wdt0.callback = wdt_int_cb;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.window.max = WDT_WINDOW_MAX;
	m_cfg_wdt0.window.min = 0U;
	my_wdt_channel = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
	if (my_wdt_channel < 0) {
		LOG_ERR("wdt_install_timeout() returned %d", my_wdt_channel);
		atomic_inc(&completed_threads);
		return;
	}

	ret = wdt_setup(my_wdt_device, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (ret < 0) {
		LOG_ERR("wdt_setup() returned %d", ret);
		atomic_inc(&completed_threads);
		return;
	}

	for (int i = 0; i < WDT_THREAD_COUNT_MAX; i++) {

		/* Feed watchdog */
		ret = wdt_feed(my_wdt_device, my_wdt_channel);
		if (ret < 0) {
			LOG_ERR("wdt_feed() returned: %d", ret);
			/* Stop watchdog */
			wdt_disable(my_wdt_device);
			return;
		}

		LOG_INF("WDT was feed");
		k_msleep(WDT_THREAD_PERIOD);
	}

	/* Stop watchdog */
	wdt_disable(my_wdt_device);

	LOG_INF("WDT thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_wdt_id, WDT_THREAD_STACKSIZE, wdt_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(WDT_THREAD_PRIORITY), 0, 0);
