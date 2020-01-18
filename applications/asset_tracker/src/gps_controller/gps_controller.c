/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <sys/util.h>
#include <drivers/gps.h>
#include <lte_lc.h>

#include "ui.h"
#include "gps_controller.h"


#include <logging/log.h>
LOG_MODULE_REGISTER(gps_control, CONFIG_GPS_CONTROL_LOG_LEVEL);

#if !defined(CONFIG_GPS_SIM)
/* Structure to hold GPS work information */
static struct {
	enum {
		GPS_WORK_START,
		GPS_WORK_STOP
	} type;
	struct k_delayed_work work;
	struct device *dev;
} gps_work;

static atomic_t gps_is_active;
static atomic_t gps_is_enabled;

static int start(void)
{
	int err;

	if (IS_ENABLED(CONFIG_GPS_CONTROL_PSM_ENABLE_ON_START)) {
		printk("Enabling PSM\n");

		err = lte_lc_psm_req(true);
		if (err) {
			printk("PSM mode could not be enabled");
			printk(" or was already enabled\n.");
		} else {
			printk("PSM enabled\n");
		}
	}

	err = gps_start(gps_work.dev);
	if (err) {
		printk("Failed starting GPS!\n");
		return err;
	}

	atomic_set(&gps_is_active, 1);

	printk("GPS started successfully.\nSearching for satellites ");
	printk("to get position fix. This may take several minutes.\n");
	printk("The device will attempt to get a fix for %d seconds, ",
		CONFIG_GPS_CONTROL_FIX_TRY_TIME);
	printk("before the GPS is stopped.\n");

	return 0;
}

static int stop(void)
{
	int err;

	if (IS_ENABLED(CONFIG_GPS_CONTROL_PSM_DISABLE_ON_STOP)) {
		printk("Disabling PSM\n");

		err = lte_lc_psm_req(false);
		if (err) {
			printk("PSM mode could not be disabled\n");
		}
	}

	err = gps_stop(gps_work.dev);
	if (err) {
		return err;
	}

	return 0;
}

static void gps_work_handler(struct k_work *work)
{
	int err;

	if (gps_work.type == GPS_WORK_START) {
		err = start();
		if (err) {
			printk("GPS could not be started, error: %d\n", err);
			return;
		}

		printk("GPS operation started\n");

		atomic_set(&gps_is_active, 1);
		ui_led_set_pattern(UI_LED_GPS_SEARCHING);

		gps_work.type = GPS_WORK_STOP;

		k_delayed_work_submit(&gps_work.work,
				K_SECONDS(CONFIG_GPS_CONTROL_FIX_TRY_TIME));

		return;
	} else if (gps_work.type == GPS_WORK_STOP) {
		err = stop();
		if (err) {
			printk("GPS could not be stopped, error: %d\n", err);
			return;
		}

		printk("GPS operation was stopped\n");

		atomic_set(&gps_is_active, 0);

		if (atomic_get(&gps_is_enabled) == 0) {
			return;
		}

		gps_work.type = GPS_WORK_START;


		printk("The device will try to get fix again in %d seconds\n",
			CONFIG_GPS_CONTROL_FIX_CHECK_INTERVAL);

		k_delayed_work_submit(&gps_work.work,
			K_SECONDS(CONFIG_GPS_CONTROL_FIX_CHECK_INTERVAL));
	}
}
#endif /* !defined(GPS_SIM) */

bool gps_control_is_active(void)
{
#if !defined(CONFIG_GPS_SIM)
	return atomic_get(&gps_is_active);
#else
	return false;
#endif
}

bool gps_control_is_enabled(void)
{
#if !defined(CONFIG_GPS_SIM)
	return atomic_get(&gps_is_enabled);
#else
	return false;
#endif
}

void gps_control_enable(void)
{
#if !defined(CONFIG_GPS_SIM)
	atomic_set(&gps_is_enabled, 1);
	gps_control_start(K_SECONDS(1));
#endif
}

void gps_control_disable(void)
{
#if !defined(CONFIG_GPS_SIM)
	atomic_set(&gps_is_enabled, 0);
	gps_control_stop(K_NO_WAIT);
	ui_led_set_pattern(UI_CLOUD_CONNECTED);
#endif
}

void gps_control_stop(u32_t delay_ms)
{
#if !defined(CONFIG_GPS_SIM)
	k_delayed_work_cancel(&gps_work.work);
	gps_work.type = GPS_WORK_STOP;
	k_delayed_work_submit(&gps_work.work, delay_ms);
#endif
}

void gps_control_start(u32_t delay_ms)
{
#if !defined(CONFIG_GPS_SIM)
	k_delayed_work_cancel(&gps_work.work);
	gps_work.type = GPS_WORK_START;
	k_delayed_work_submit(&gps_work.work, delay_ms);
#endif
}

void gps_control_on_trigger(void)
{
#if !defined(CONFIG_GPS_SIM)

#endif
}

/** @brief Configures and starts the GPS device. */
int gps_control_init(gps_trigger_handler_t handler)
{
	int err;
	struct device *gps_dev;

#ifdef CONFIG_GPS_SIM
	struct gps_trigger gps_trig = {
		.type = GPS_TRIG_DATA_READY
	};
#else
	struct gps_trigger gps_trig = {
		.type = GPS_TRIG_FIX,
		.chan = GPS_CHAN_NMEA
	};
#endif /* CONFIG_GPS_SIM */

	gps_dev = device_get_binding(CONFIG_GPS_DEV_NAME);
	if (gps_dev == NULL) {
		printk("Could not get %s device\n", CONFIG_GPS_DEV_NAME);
		return -ENODEV;
	}


	err = gps_trigger_set(gps_dev, &gps_trig, handler);
	if (err) {
		printk("Could not set trigger, error code: %d\n", err);
		return err;
	}

#if !defined(CONFIG_GPS_SIM)
	k_delayed_work_init(&gps_work.work, gps_work_handler);

	gps_work.dev = gps_dev;
#endif
	printk("GPS initialized\n");

	return 0;
}
