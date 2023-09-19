/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <modem/location.h>
#include <net/wifi_location_common.h>

#include "location_core.h"
#include "location_utils.h"
#include "scan_cellular.h"
#include "scan_wifi.h"
#include "cloud_service/cloud_service.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

BUILD_ASSERT(
	IS_ENABLED(CONFIG_LOCATION_SERVICE_NRF_CLOUD) ||
	IS_ENABLED(CONFIG_LOCATION_SERVICE_HERE) ||
	IS_ENABLED(CONFIG_LOCATION_SERVICE_EXTERNAL),
	"At least one location service, or handling the service externally must be enabled");

/* Common for both */
struct method_cloud_location_start_work_args {
	struct k_work work_item;
	const struct location_wifi_config *wifi_config;
	const struct location_cellular_config *cell_config;
	int64_t locreq_timeout_uptime;
};

static struct method_cloud_location_start_work_args method_cloud_location_start_work;
static bool running;

static void method_cloud_location_positioning_work_fn(struct k_work *work)
{
	struct method_cloud_location_start_work_args *work_data =
		CONTAINER_OF(work, struct method_cloud_location_start_work_args, work_item);
	const struct location_wifi_config *wifi_config = work_data->wifi_config;
	const struct location_cellular_config *cell_config = work_data->cell_config;
	struct wifi_scan_info *scan_wifi_info = NULL;
	struct lte_lc_cells_info *scan_cellular_info = NULL;
	int err = 0;
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	struct k_sem wifi_scan_ready;

	k_sem_init(&wifi_scan_ready, 0, 1);

	if (wifi_config != NULL) {
		scan_wifi_execute(wifi_config->timeout, &wifi_scan_ready);
	}
#endif

#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
	if (cell_config != NULL) {
		scan_cellular_execute(cell_config->timeout, cell_config->cell_count);
		scan_cellular_info = scan_cellular_results_get();
	}
#endif

#if defined(CONFIG_LOCATION_METHOD_WIFI)
	if (wifi_config != NULL) {
		k_sem_take(&wifi_scan_ready, K_FOREVER);
		scan_wifi_info = scan_wifi_results_get();
	}
#endif

	if (!running) {
		goto end;
	}

	if (scan_cellular_info == NULL && scan_wifi_info == NULL) {
		LOG_WRN("No cellular neighbor cells or Wi-Fi access points found");
		err = -ENODATA;
		goto end;
	}

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	ARG_UNUSED(wifi_config);

	struct location_data_cloud request = {
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
		.cell_data = scan_cellular_info,
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		.wifi_data = scan_wifi_info,
#endif
	};

	location_core_event_cb_cloud_location_request(&request);
	return;
#else
	struct location_data location;
	struct location_data location_result = { 0 };
	struct cloud_service_pos_req params = {
		.cell_data = scan_cellular_info,
		.wifi_data = scan_wifi_info,
		.service = (cell_config != NULL) ? cell_config->service : wifi_config->service,
		.timeout_ms = SYS_FOREVER_MS
	};

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB) && !location_utils_is_lte_available()) {
		/* Not worth to start trying to fetch the location over LTE.
		 * Thus, fail faster in this case and save the trying "costs".
		 */
		LOG_WRN("Default PDN context is NOT active, cannot retrieve a location");
		err = -EFAULT;
		goto end;
	}

	/* Scannings done at this point of time. Store current time to response. */
	location_utils_systime_to_location_datetime(&location_result.datetime);

	/* Timeout for cloud request is the remaining time from the location request timeout.
	 * Notice that it's not from the method timeout, which only applies to the scan procedure.
	 */
	if (work_data->locreq_timeout_uptime != SYS_FOREVER_MS) {
		params.timeout_ms = work_data->locreq_timeout_uptime - k_uptime_get();
		if (params.timeout_ms < 0) {
			LOG_WRN("Timeout occurred during scannings");
			err = -ETIMEDOUT;
			goto end;
		}
	}

	/* Request location from the cloud */
	err = cloud_service_location_get(&params, &location);
	if (err) {
		LOG_ERR("Failed to acquire location using cloud location, error: %d", err);
	} else {
		location_result.latitude = location.latitude;
		location_result.longitude = location.longitude;
		location_result.accuracy = location.accuracy;
		location_core_event_cb(&location_result);
	}

#endif /* defined(CONFIG_LOCATION_SERVICE_EXTERNAL) */

end:
	if (err == -ETIMEDOUT) {
		location_core_event_cb_timeout();
	} else if (err) {
		location_core_event_cb_error();
	}
	running = false;
}

int method_cloud_location_cancel(void)
{
	if (running) {
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		scan_wifi_cancel();
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
		scan_cellular_cancel();
#endif
		(void)k_work_cancel(&method_cloud_location_start_work.work_item);

		running = false;
	} else {
		return -EPERM;
	}
	return 0;
}

int method_cloud_location_get(const struct location_request_info *request)
{
	__ASSERT_NO_MSG(request->cellular != NULL || request->wifi != NULL);

	k_work_init(
		&method_cloud_location_start_work.work_item,
		method_cloud_location_positioning_work_fn);

	/* Select configurations based on requested method */
	method_cloud_location_start_work.wifi_config = NULL;
	method_cloud_location_start_work.cell_config = NULL;
	if (request->current_method == LOCATION_METHOD_CELLULAR ||
	    request->current_method == LOCATION_METHOD_INTERNAL_WIFI_CELLULAR) {
		method_cloud_location_start_work.cell_config = request->cellular;
	}
	if (request->current_method == LOCATION_METHOD_WIFI ||
	    request->current_method == LOCATION_METHOD_INTERNAL_WIFI_CELLULAR) {
		method_cloud_location_start_work.wifi_config = request->wifi;
	}

	method_cloud_location_start_work.locreq_timeout_uptime = request->timeout_uptime;
	k_work_submit_to_queue(
		location_core_work_queue_get(),
		&method_cloud_location_start_work.work_item);

	running = true;

	return 0;
}

int method_cloud_location_init(void)
{
	running = false;

#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	cloud_service_init();
#endif

	return 0;
}
