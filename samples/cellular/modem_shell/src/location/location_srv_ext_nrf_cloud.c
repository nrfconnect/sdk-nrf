/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cJSON.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_agps.h>
#include <net/nrf_cloud_pgps.h>
#include <net/nrf_cloud_location.h>
#include <net/nrf_cloud_codec.h>
#include <modem/location.h>

#include "mosh_print.h"
#include "mosh_defines.h"
#include "location_srv_ext.h"

static struct location_data nrf_cloud_location;
static K_SEM_DEFINE(location_ready, 0, 1);

#if defined(CONFIG_NRF_CLOUD_AGPS)
void location_srv_ext_agps_handle(const struct nrf_modem_gnss_agps_data_frame *agps_req)
{
	int err;

	err = nrf_cloud_agps_request(agps_req);
	if (err) {
		mosh_error("nRF Cloud A-GPS request failed, error: %d", err);
		return;
	}

	mosh_print("A-GPS data requested");
}
#endif /* CONFIG_NRF_CLOUD_AGPS */

#if defined(CONFIG_NRF_CLOUD_PGPS)

void location_srv_ext_pgps_handle(const struct gps_pgps_request *pgps_req)
{
	int err = 0;
	NRF_CLOUD_OBJ_JSON_DEFINE(pgps_obj);

	err = nrf_cloud_obj_pgps_request_create(&pgps_obj, pgps_req);
	if (err) {
		mosh_error("Could not create P-GPS request message, error: %d", err);
		goto cleanup;
	}

	struct nrf_cloud_tx_data mqtt_msg = {
		.obj = &pgps_obj,
		.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
	};

	err = nrf_cloud_send(&mqtt_msg);
	if (!err) {
		mosh_print("P-GPS data requested");
	}

cleanup:
	(void)nrf_cloud_obj_free(&pgps_obj);

	if (err) {
		mosh_error("nRF Cloud P-GPS request failed, error: %d", err);
	}
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

#if defined(CONFIG_LOCATION_METHOD_CELLULAR) || defined(CONFIG_LOCATION_METHOD_WIFI)
static void location_srv_ext_nrf_cloud_location_ready_cb(
	const struct nrf_cloud_location_result *const result)
{
	if ((result != NULL) && (result->err == NRF_CLOUD_ERROR_NONE)) {
		nrf_cloud_location.latitude = result->lat;
		nrf_cloud_location.longitude = result->lon;
		nrf_cloud_location.accuracy = (double)result->unc;

		k_sem_give(&location_ready);
	} else {
		if (result) {
			mosh_error(
				"Unable to determine location from cloud response, error: %d",
				result->err);
		}
		/* Reset the semaphore to unblock location_srv_ext_nrf_cloud_location_get()
		 * and make it return an error.
		 */
		k_sem_reset(&location_ready);
	}
}

static int location_srv_ext_nrf_cloud_location_get(
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
	const struct lte_lc_cells_info *cell_data,
#else
	void *cell_data,
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	const struct wifi_scan_info *scanning_results,
#else
	void *scanning_results,
#endif
	struct location_data *const location)
{
	int err;
	nrf_cloud_location_response_t callback = NULL;

	k_sem_reset(&location_ready);

	if (location) {
		callback = location_srv_ext_nrf_cloud_location_ready_cb;
	}

	mosh_print("Sending positioning request to cloud via MQTT");
	err = nrf_cloud_location_request(
		cell_data, scanning_results, (callback != NULL), callback);
	if (err == -EACCES) {
		mosh_error("Cloud connection is not established");
		return err;
	} else if (err) {
		mosh_error("Failed to request positioning data, error: %d", err);
		return err;
	}

	if (callback) {
		/* We are in the middle of the Cellular/Wi-Fi positioning and we have received
		 * an event from Location library for handling the cloud transaction.
		 * Location library has stopped its method-specific timeout.
		 * We could set cellular_timeout or wifi_timeout into the timeout of k_sem_take()
		 * but we have opted to simplify implementation of the application and use
		 * a separate timeout for a cloud transaction.
		 */
		if (k_sem_take(&location_ready, K_MSEC(30000)) == -EAGAIN) {
			mosh_error(
				"Positioning data request timed out or "
				"cloud did not return a location");
			return -ETIMEDOUT;
		}

		*location = nrf_cloud_location;
	}

	return err;
}
#endif /* CONFIG_LOCATION_METHOD_CELLULAR || CONFIG_LOCATION_METHOD_WIFI */

#if defined(CONFIG_LOCATION_METHOD_CELLULAR) || defined(CONFIG_LOCATION_METHOD_WIFI)
void location_srv_ext_cloud_location_handle(
	const struct location_data_cloud *cloud_location_request,
	bool cloud_resp_enabled)
{
	int err;
	struct location_data *result = NULL;

	if (cloud_resp_enabled) {
		result = &nrf_cloud_location;
	}
	err = location_srv_ext_nrf_cloud_location_get(
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
		cloud_location_request->cell_data,
#else
		NULL,
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		cloud_location_request->wifi_data,
#else
		NULL,
#endif
		result);
	if (err) {
		mosh_error("Failed to send cloud location request, err: %d", err);
		location_cloud_location_ext_result_set(LOCATION_EXT_RESULT_ERROR, NULL);
	} else if (cloud_resp_enabled) {
		location_cloud_location_ext_result_set(
			LOCATION_EXT_RESULT_SUCCESS, &nrf_cloud_location);
	} else {
		location_cloud_location_ext_result_set(LOCATION_EXT_RESULT_UNKNOWN, NULL);
	}
}
#endif /* CONFIG_LOCATION_METHOD_CELLULAR && CONFIG_LOCATION_METHOD_WIFI */
