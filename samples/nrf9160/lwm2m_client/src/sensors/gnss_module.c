/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrf_modem_gnss.h>
#include <app_event_manager.h>
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
#include <net/lwm2m_client_utils_location.h>
#endif

#include "gnss_module.h"
#include "gnss_pvt_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gnss_module, CONFIG_APP_LOG_LEVEL);

static struct nrf_modem_gnss_pvt_data_frame pvt_data;
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
static struct nrf_modem_gnss_agps_data_frame agps_req;
#endif
static bool running;

/**@brief Callback for GNSS events */
static void gnss_event_handler(int event_id)
{
	int err = 0;

	switch (event_id) {
	case NRF_MODEM_GNSS_EVT_FIX: {
		LOG_INF("GNSS fix available");
		err = nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT);
		if (err) {
			LOG_ERR("Error reading PVT data (%d)", err);
			return;
		}

		struct gnss_pvt_event *event = new_gnss_pvt_event();

		event->pvt = pvt_data;
		APP_EVENT_SUBMIT(event);
		break;
		}
	case NRF_MODEM_GNSS_EVT_AGPS_REQ: {
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
		LOG_INF("GPS requests AGPS Data. Sending request to LwM2M server");
		err = nrf_modem_gnss_read(&agps_req, sizeof(agps_req),
					  NRF_MODEM_GNSS_DATA_AGPS_REQ);
		if (err) {
			LOG_ERR("Error reading AGPS request (%d)", err);
		}

		struct gnss_agps_request_event *event = new_gnss_agps_request_event();

		event->agps_req = agps_req;
		APP_EVENT_SUBMIT(event);
#else
		LOG_DBG("A-GPS data request not supported.");
#endif
		break;
		}
	case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
		LOG_DBG("GNSS search started");
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_DBG("GNSS timed out waiting for fix");
		break;
	default:
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
