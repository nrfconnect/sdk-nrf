/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/watchdog.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt, LOG_LEVEL_INF);

#define WDT_DELAY		(200)
#define WDT_WINDOW_MAX	(3 * WDT_DELAY)
#define WDT_ITERATIONS	(65535)

static const struct device *const my_wdt_device = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static struct wdt_timeout_cfg m_cfg_wdt0;
static int wdt_chan0, wdt_chan1, wdt_chan2, wdt_chan3;

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
#define NOINIT_SECTION ".dtcm_noinit.test_wdt"
#else
#define NOINIT_SECTION ".noinit.test_wdt"
#endif
volatile uint32_t wdt_status __attribute__((section(NOINIT_SECTION)));

static void wdt_int_cb0(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	LOG_INF("CB0");
}

static void wdt_int_cb1(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	LOG_INF("CB1");
}

static void wdt_int_cb2(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	LOG_INF("CB2");
}

static void wdt_int_cb3(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	LOG_INF("CB3");
}


/* Timer */
#define TIMER_PERIOD	(49)

static struct k_timer my_timer;
static int timer_expire_count;

void my_work_handler(struct k_work *work)
{
	timer_expire_count++;
	LOG_INF("Timer expired %d times", timer_expire_count);
}

K_WORK_DEFINE(my_work, my_work_handler);

void my_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&my_work);
}


/* Counter */
#define ALARM_CHANNEL_ID	(0)
#define COUNTER_PERIOD		(9)

static const struct device *const counter_dev = DEVICE_DT_GET(DT_ALIAS(test_counter));
static struct counter_alarm_cfg alarm_cfg;
static bool counter_enabled = true;

static void my_counter_cb(const struct device *counter_dev,
	uint8_t chan_id, uint32_t ticks, void *user_data)
{
	int ret;

	/* Set a new alarm */
	if (counter_enabled) {
		ret = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID, user_data);
		if (ret != 0) {
			LOG_ERR("counter_set_channel_alarm() failed (%d)", ret);
		}
	} else {
		counter_stop(counter_dev);
	}
}


int main(void)
{
	int ret;

	LOG_INF("Test executed on %s", CONFIG_BOARD_TARGET);

	if (!device_is_ready(my_wdt_device)) {
		LOG_ERR("WDT device %s is not ready", my_wdt_device->name);
		return -1;
	}

	if (!device_is_ready(counter_dev)) {
		LOG_ERR("Device %s is not ready.", counter_dev->name);
		return -101;
	}

	/* start a periodic timer that expires once every TIMER_PERIOD */
	k_timer_init(&my_timer, my_timer_handler, NULL);
	k_timer_start(&my_timer, K_MSEC(TIMER_PERIOD), K_MSEC(TIMER_PERIOD));

	/* Configure counter */
	counter_start(counter_dev);

	alarm_cfg.flags = 0;
	alarm_cfg.ticks = counter_us_to_ticks(counter_dev, COUNTER_PERIOD * 1000);
	alarm_cfg.callback = my_counter_cb;
	alarm_cfg.user_data = &alarm_cfg;

	ret = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID, &alarm_cfg);
	if (ret != 0) {
		LOG_ERR("counter_set_channel_alarm() failed (%d)", ret);
	}

	for (int i = 0; i < WDT_ITERATIONS; i++) {
		LOG_INF("Iteration %d", i);

		/* Configure Watchdog */
		m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
		m_cfg_wdt0.window.max = WDT_WINDOW_MAX;
		m_cfg_wdt0.window.min = 0U;

		m_cfg_wdt0.callback = wdt_int_cb0;
		wdt_chan0 = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
		if (wdt_chan0 < 0) {
			LOG_ERR("wdt_install_timeout() returned %d", wdt_chan0);
			return -2;
		}

		m_cfg_wdt0.callback = wdt_int_cb1;
		wdt_chan1 = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
		if (wdt_chan1 < 0) {
			LOG_ERR("wdt_install_timeout() returned %d", wdt_chan1);
			return -3;
		}

		m_cfg_wdt0.callback = wdt_int_cb2;
		wdt_chan2 = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
		if (wdt_chan2 < 0) {
			LOG_ERR("wdt_install_timeout() returned %d", wdt_chan2);
			return -4;
		}

		m_cfg_wdt0.callback = wdt_int_cb3;
		wdt_chan3 = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
		if (wdt_chan3 < 0) {
			LOG_ERR("wdt_install_timeout() returned %d", wdt_chan3);
			return -5;
		}

		/* Start Watchdog */
		ret = wdt_setup(my_wdt_device, WDT_OPT_PAUSE_HALTED_BY_DBG);
		if (ret < 0) {
			LOG_ERR("wdt_setup() returned %d", ret);
			return -6;
		}

		/* Feed watchdog */
		ret = wdt_feed(my_wdt_device, wdt_chan0);
		if (ret < 0) {
			LOG_ERR("wdt_feed(wdt_chan0) returned: %d", ret);
			return -7;
		}

		ret = wdt_feed(my_wdt_device, wdt_chan1);
		if (ret < 0) {
			LOG_ERR("wdt_feed(wdt_chan1) returned: %d", ret);
			return -8;
		}

		ret = wdt_feed(my_wdt_device, wdt_chan2);
		if (ret < 0) {
			LOG_ERR("wdt_feed(wdt_chan2) returned: %d", ret);
			return -9;
		}

		ret = wdt_feed(my_wdt_device, wdt_chan3);
		if (ret < 0) {
			LOG_ERR("wdt_feed(wdt_chan3) returned: %d", ret);
			return -10;
		}

		k_msleep(WDT_DELAY);

		/* Stop watchdog */
		wdt_disable(my_wdt_device);
	}

	k_timer_stop(&my_timer);

	counter_enabled = false;

	LOG_INF("WDT test has completed");
}
