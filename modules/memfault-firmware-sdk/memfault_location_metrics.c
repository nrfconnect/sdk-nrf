/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <modem/location.h>

#include "memfault/metrics/metrics.h"
#include "memfault_location_metrics.h"

LOG_MODULE_DECLARE(memfault_ncs_metrics, CONFIG_MEMFAULT_NCS_LOG_LEVEL);

static bool s_session_in_progress;
static bool s_cloud_request_captured;

/**
 * @brief Record location method results
 *
 * @param method location method
 * @param id location event id
 * @param details pointer to location data details
 * @param accuracy_m pointer to location accuracy in meters
 */
static void prv_location_method_results_record(enum location_method method,
					       enum location_event_id id,
					       const struct location_data_details *details,
					       const float *accuracy_m)
{
	switch (method) {
#if defined(CONFIG_LOCATION_METHOD_GNSS)
	case LOCATION_METHOD_GNSS:
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_gnss_method_time_ms, ncs_loc,
						     details->elapsed_time_method);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_gnss_method_result, ncs_loc, id);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_gnss_satellites_tracked_count, ncs_loc,
						     details->gnss.satellites_tracked);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_gnss_satellites_used_count, ncs_loc,
						     details->gnss.satellites_used);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_gnss_on_time_ms, ncs_loc,
						     details->gnss.elapsed_time_gnss);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_gnss_exec_time_ms, ncs_loc,
						     details->gnss.pvt_data.execution_time);

		/* Only record TTF and accuracy when a fix is acquired */
		if (id == LOCATION_EVT_LOCATION) {
			MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_gnss_time_to_fix_ms, ncs_loc,
							     details->gnss.pvt_data.execution_time);

			if (accuracy_m != NULL) {
				uint32_t accuracy_cm = (uint32_t)(*accuracy_m * 100.0f);

				MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_accuracy_cm, ncs_loc,
								     accuracy_cm);
			}
		}
		break;
#endif /* CONFIG_LOCATION_METHOD_GNSS */
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
	case LOCATION_METHOD_CELLULAR:
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_lte_method_time_ms, ncs_loc,
						     details->elapsed_time_method);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_lte_method_result, ncs_loc, id);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_lte_neighbor_cells_count, ncs_loc,
						     details->cellular.ncells_count);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_lte_gci_cells_count, ncs_loc,
						     details->cellular.gci_cells_count);
		break;
#endif /* CONFIG_LOCATION_METHOD_CELLULAR */
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	case LOCATION_METHOD_WIFI:
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_wifi_method_time_ms, ncs_loc,
						     details->elapsed_time_method);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_wifi_method_result, ncs_loc, id);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_wifi_ap_count, ncs_loc,
						     details->wifi.ap_count);
		break;
#endif /* CONFIG_LOCATION_METHOD_CELLULAR && CONFIG_LOCATION_METHOD_WIFI */
#if defined(CONFIG_LOCATION_METHOD_CELLULAR) && defined(CONFIG_LOCATION_METHOD_WIFI)
	case LOCATION_METHOD_WIFI_CELLULAR:
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_lte_method_time_ms, ncs_loc,
						     details->elapsed_time_method);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_lte_method_result, ncs_loc, id);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_lte_neighbor_cells_count, ncs_loc,
						     details->cellular.ncells_count);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_lte_gci_cells_count, ncs_loc,
						     details->cellular.gci_cells_count);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_wifi_method_time_ms, ncs_loc,
						     details->elapsed_time_method);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_wifi_method_result, ncs_loc, id);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_wifi_ap_count, ncs_loc,
						     details->wifi.ap_count);
		break;
#endif /* CONFIG_LOCATION_METHOD_CELLULAR && CONFIG_LOCATION_METHOD_WIFI */
	default:
		LOG_ERR("Unsupported location method id=%d", method);
		break;
	}
}

/**
 * @brief Record the fallback method used
 *
 * @param method fallback method
 * @param fallback pointer to fallback method data
 */
static void prv_location_method_fallback_record(enum location_method method,
						const struct location_data_fallback *fallback)
{
	prv_location_method_results_record(method, fallback->cause, &fallback->details, NULL);

	/* Log the results */
	char method_str[16];

	snprintk(method_str, sizeof(method_str), "%s", location_method_str(method));
	LOG_DBG("%s method attempted", method_str);

	char fallback_method_str[16];

	snprintk(fallback_method_str, sizeof(fallback_method_str), "%s",
		 location_method_str(fallback->next_method));
	LOG_DBG("Falling back to %s method", fallback_method_str);
}

/**
 * @brief Record cloud location request data
 *
 * @param request pointer to cloud location request data
 */
static void prv_cloud_location_request_record(const struct location_data_cloud *request)
{
	s_cloud_request_captured = true;

#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
	if (request->cell_data != NULL) {
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(
			ncs_loc_lte_neighbor_cells_count, ncs_loc,
			request->cell_data->ncells_count);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(
			ncs_loc_lte_gci_cells_count, ncs_loc,
			request->cell_data->gci_cells_count);
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	if (request->wifi_data != NULL) {
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(
			ncs_loc_wifi_ap_count, ncs_loc,
			request->wifi_data->cnt);
	}
#endif
}

/**
 * @brief Start a location session.
 *
 */
static void prv_location_metrics_session_start(void)
{
	if (s_session_in_progress) {
		LOG_WRN("Location session already in progress, ignoring start request");
		return;
	}

	s_session_in_progress = true;
	s_cloud_request_captured = false;
	LOG_DBG("Starting location session");
	MEMFAULT_METRICS_SESSION_START(ncs_loc);
	MEMFAULT_METRIC_ADD(ncs_loc_search_request_count, 1);
}

/**
 * @brief Stop a location fix session.
 *
 * @param event_data pointer to location event data
 */
static void prv_location_metrics_session_stop(const struct location_event_data *event_data)
{
	if (!s_session_in_progress) {
		LOG_WRN("No location session in progress, ignoring stop request");
		return;
	}

	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		LOG_DBG("Stopping location session, fix data acquired");
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(
			ncs_loc_search_time_ms, ncs_loc,
			event_data->location.details.elapsed_time_method);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_search_success, ncs_loc, 1);

		prv_location_method_results_record(event_data->method, event_data->id,
						   &event_data->location.details,
						   &event_data->location.accuracy);
		break;
	case LOCATION_EVT_CANCELLED:
		if (s_cloud_request_captured) {
			/* Cloud request captured before cancel - consider success as app
			 * received Wi-Fi/cellular data for cloud location.
			 */

			MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_search_success, ncs_loc, 1);
		} else {
			MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_search_failure, ncs_loc, 1);
		}
		break;
	case LOCATION_EVT_TIMEOUT:
		/* Intentional fall through */
	case LOCATION_EVT_ERROR:
		/* Intentional fall through */
	case LOCATION_EVT_RESULT_UNKNOWN:
		/* Intentional fall through */
	default:
		LOG_DBG("Stopping location session, no fix found, id=%d", event_data->id);
		MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ncs_loc_search_failure, ncs_loc, 1);
		prv_location_method_results_record(event_data->method, event_data->id,
						   &event_data->error.details, NULL);
		break;
	}

	MEMFAULT_METRICS_SESSION_END(ncs_loc);
	s_session_in_progress = false;
}

/**
 * @brief Handle location events
 *
 * @param event_data pointer to location event data
 */
static void prv_location_event_handler(const struct location_event_data *event_data)
{
	if (event_data->id == LOCATION_EVT_STARTED) {
		prv_location_metrics_session_start();
	} else if (event_data->id == LOCATION_EVT_FALLBACK) {
		prv_location_method_fallback_record(event_data->method, &event_data->fallback);
	} else if (event_data->id == LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST) {
		prv_cloud_location_request_record(&event_data->cloud_location_request);
	} else if (event_data->id == LOCATION_EVT_LOCATION ||
		   event_data->id == LOCATION_EVT_TIMEOUT || event_data->id == LOCATION_EVT_ERROR ||
		   event_data->id == LOCATION_EVT_RESULT_UNKNOWN ||
		   event_data->id == LOCATION_EVT_CANCELLED) {
		prv_location_metrics_session_stop(event_data);
	}
}

void memfault_location_metrics_init(void)
{
	int result = location_handler_register(prv_location_event_handler);

	if (result != 0) {
		LOG_ERR("Error registering location handler, err %d", result);
	}
}
