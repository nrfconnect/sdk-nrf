/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "wdt_test.h"

LOG_MODULE_REGISTER(wdt_test, LOG_LEVEL_INF);

static const struct device *const my_wdt_device = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static struct wdt_timeout_cfg m_cfg_wdt0;
static int my_wdt_channel;

extern atomic_t started_threads;

static void wdt_callback(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
}

static int setup(void)
{
	int ret;

	m_cfg_wdt0.callback = wdt_callback;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.window.max = WDT_WINDOW_MAX;
	m_cfg_wdt0.window.min = 0U;

	ret = device_is_ready(my_wdt_device);
	if (ret != 1) {
		LOG_ERR("WDT device is not ready: %d", ret);
		return -1;
	}

	my_wdt_channel = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
	if (my_wdt_channel < 0) {
		LOG_ERR("WDT install timeout failed %d", my_wdt_channel);
		return -2;
	}

	ret = wdt_setup(my_wdt_device, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (ret < 0) {
		LOG_ERR("WDT setup %d", ret);
		return -3;
	}

	return 0;
}

static void wdt_load_thread_worker(void *arg1, void *arg2, void *arg3)
{
	int ret;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	ret = setup();
	if (ret != 0) {
		LOG_ERR("WDT setup failed: %d\n", ret);
		return;
	}

	atomic_inc(&started_threads);

	LOG_INF("WDT load thread started");
	while (1) {
		ret = wdt_feed(my_wdt_device, my_wdt_channel);
		if (ret < 0) {
			LOG_ERR("WDT feed failed: %d", ret);
		}
		k_msleep(WDT_THREAD_SLEEP);
	}
}

K_THREAD_DEFINE(wdt_load_thread, WDT_THREAD_STACKSIZE, wdt_load_thread_worker, NULL, NULL, NULL,
		K_PRIO_PREEMPT(WDT_THREAD_PRIORITY), 0, 0);
