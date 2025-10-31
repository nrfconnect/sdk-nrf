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
#endif

static void ntn_evt_handler(const struct ntn_evt *evt)
{
	switch (evt->type) {
	case NTN_EVT_MODEM_LOCATION_UPDATE:
		mosh_print("NTN modem location updates requested: %s",
			   evt->modem_location.updates_requested ? "true" : "false");
#if defined(CONFIG_GNSS)
		if (evt->modem_location.updates_requested) {
			ntn_modem_location_update_enable();
		} else {
			ntn_modem_location_update_disable();
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
	if (update_enabled && data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
		ntn_modem_location_update(
			data->nav_data.latitude / NANODEG_PER_DEG,
			data->nav_data.longitude / NANODEG_PER_DEG,
			data->nav_data.altitude / MM_PER_M,
			data->nav_data.speed / MM_PER_M);
	}
}

GNSS_DATA_CALLBACK_DEFINE(GNSS_DEV, gnss_data_cb);
#endif /* defined(CONFIG_GNSS) */

int ntn_modem_location_update_enable(void)
{
#if defined(CONFIG_GNSS)
	mosh_print("NTN modem location updates from external GNSS enabled");

	update_enabled = true;

	return 0;
#else
	mosh_error("External GNSS not available, enable CONFIG_GNSS");

	return -ENOEXEC;
#endif /* defined(CONFIG_GNSS) */
}

int ntn_modem_location_update_disable(void)
{
#if defined(CONFIG_GNSS)
	mosh_print("NTN modem location updates from external GNSS disabled");

	update_enabled = false;

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
