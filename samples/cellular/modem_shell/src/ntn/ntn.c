/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/pm/device.h>
#include <zephyr/drivers/gnss.h>

#include <modem/ntn.h>

#include "mosh_print.h"
#include "ntn.h"

#if defined(CONFIG_GNSS)
#define GNSS_DEV DEVICE_DT_GET(DT_ALIAS(gnss))
#endif

#define NANODEG_PER_DEG	1000000000.0
#define MM_PER_M	1000.0

#if defined(CONFIG_GNSS)
static bool update_enabled;
static uint16_t update_interval;
static uint64_t update_timestamp;

static uint32_t update_interval_calculate(uint32_t accuracy)
{
	uint32_t interval;

	/* Calculate update interval required to maintain accuracy at 120 km/h speed. */
	interval = (accuracy / 33.333);
	/* Make sure update interval is not shorter than 5 seconds. */
	interval = MAX(interval, 5);

	return interval;
}
#endif

static void ntn_evt_handler(const struct ntn_evt *evt)
{
	switch (evt->type) {
	case NTN_EVT_LOCATION_REQUEST:
		mosh_print("NTN location request event: requested: %s, accuracy: %d m",
			   evt->location_request.requested ? "true" : "false",
			   evt->location_request.accuracy);
#if defined(CONFIG_GNSS)
		if (evt->location_request.requested) {
			ntn_location_update_enable(
				update_interval_calculate(evt->location_request.accuracy));
		} else {
			ntn_location_update_disable();
		}
#endif /* defined(CONFIG_GNSS) */
		break;

	default:
		mosh_warn("Unknown NTN event: %d", evt->type);
		break;
	}
}

#if defined(CONFIG_GNSS)
static void gnss_data_cb(const struct device *dev, const struct gnss_data *data)
{
	uint64_t current_timestamp;

	if (update_enabled && data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
		current_timestamp = k_uptime_get();
		if (((current_timestamp - update_timestamp) >
		    ((update_interval - 1) * MSEC_PER_SEC)) || update_timestamp == 0) {
			update_timestamp = current_timestamp;

			ntn_location_set(
				data->nav_data.latitude / NANODEG_PER_DEG,
				data->nav_data.longitude / NANODEG_PER_DEG,
				data->nav_data.altitude / MM_PER_M,
				update_interval * 2);
		}
	}
}

GNSS_DATA_CALLBACK_DEFINE(GNSS_DEV, gnss_data_cb);
#endif /* defined(CONFIG_GNSS) */

int ntn_location_update_enable(uint32_t interval)
{
#if defined(CONFIG_GNSS)
	if (!update_enabled) {
		mosh_print("NTN location updates from external GNSS enabled");
	}

	/* Reset timestamp to force a location update immediately. */
	update_timestamp = 0;
	update_interval = interval;
	update_enabled = true;

	return 0;
#else
	mosh_error("External GNSS not available, enable CONFIG_GNSS");

	return -ENOEXEC;
#endif /* defined(CONFIG_GNSS) */
}

int ntn_location_update_disable(void)
{
#if defined(CONFIG_GNSS)
	mosh_print("NTN location updates from external GNSS disabled");

	update_enabled = false;
	update_interval = 0;

	return 0;
#else
	mosh_error("External GNSS not available, enable CONFIG_GNSS");

	return -ENOEXEC;
#endif /* defined(CONFIG_GNSS) */
}

int mosh_ntn_init(void)
{
	ntn_register_handler(ntn_evt_handler);

#if defined(CONFIG_GNSS) && defined(CONFIG_PM_DEVICE)
	/* Resume GNSS device to start receiving GNSS data. */
	return pm_device_action_run(GNSS_DEV, PM_DEVICE_ACTION_RESUME);
#else
	return 0;
#endif /* defined(CONFIG_GNSS) && defined(CONFIG_PM_DEVICE) */
}

SYS_INIT(mosh_ntn_init, APPLICATION, 0);
