/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <nrf_modem_gnss.h>
#include <app_event_manager.h>

#include "gnss_module.h"
#include "gnss_pvt_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(gnss_module, CONFIG_APP_LOG_LEVEL);

static struct nrf_modem_gnss_pvt_data_frame pvt_data;
static bool running;

/**@brief Callback for GNSS events */
static void gnss_event_handler(int event_id)
{
	int err = 0;

	switch (event_id) {
	case NRF_MODEM_GNSS_EVT_FIX:
		err = nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT);
		if (err) {
			LOG_ERR("Error reading PVT data (%d)", err);
			return;
		}

		struct gnss_pvt_event *event = new_gnss_pvt_event();

		event->pvt = pvt_data;
		APPLICATION_EVENT_SUBMIT(event);
		break;
	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
		LOG_DBG("GNSS requests AGPS Data. AGPS is not implemented.");
		break;
	case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
		LOG_DBG("GNSS search started");
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_DBG("GNSS timed out waiting for fix");
		break;
	default:
		LOG_DBG("GNSS event (%d) omitted.", event_id);
		break;
	}
}

int initialise_gnss(void)
{
	int err;

	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);

	if (err) {
		LOG_ERR("Could not set event handler for GNSS (%d)", err);
		return err;
	}

	/* This use case flag should always be set. */
	err = nrf_modem_gnss_use_case_set(NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START);

	if (err) {
		LOG_ERR("Failed to set GNSS use case (%d)", err);
		return err;
	}

	uint16_t search_interval = CONFIG_GNSS_SEARCH_INTERVAL_TIME;

	err = nrf_modem_gnss_fix_interval_set(search_interval);

	if (err) {
		LOG_ERR("Could not set fix interval for GNSS %d (%d)", search_interval, err);
		return err;
	}

	uint16_t search_timeout = CONFIG_GNSS_SEARCH_TIMEOUT_TIME;

	err = nrf_modem_gnss_fix_retry_set(search_timeout);

	if (err) {
		LOG_ERR("Could not set fix timeout for GNSS %d (%d)", search_timeout, err);
		return err;
	}

	return err;
}

int start_gnss(void)
{
	int err;

	if (running) {
		return -EALREADY;
	}

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_ERR("Could not start GNSS (%d)", err);
	} else {
		LOG_DBG("Started GNSS scan");
		running = true;
#if defined(CONFIG_GNSS_PRIORITY_ON_FIRST_FIX)
		/* Priority mode is disabled automatically after first fix */
		err = nrf_modem_gnss_prio_mode_enable();

		if (err) {
			LOG_ERR("Could not set priority mode for GNSS (%d)", err);
		}
#endif

	}
	return err;
}
