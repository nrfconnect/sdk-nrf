/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <cJSON.h>
#include <cJSON_os.h>

#include <zephyr/logging/log.h>

#include <cJSON.h>

#include <net/rest_client.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_location.h>
#if defined(CONFIG_NRF_CLOUD_REST)
#include <net/nrf_cloud_rest.h>
#endif

#include "wifi_nrf_cloud.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

/******************************************************************************/

#if defined(CONFIG_NRF_CLOUD_MQTT)
static K_SEM_DEFINE(wifi_mqtt_location_ready, 0, 1);
static struct location_data nrf_cloud_location;

static void wifi_nrf_cloud_mqtt_location_ready_cb(
	const struct nrf_cloud_location_result *const result)
{
	if ((result != NULL) && (result->err == NRF_CLOUD_ERROR_NONE)) {
		nrf_cloud_location.latitude = result->lat;
		nrf_cloud_location.longitude = result->lon;
		nrf_cloud_location.accuracy = (double)result->unc;

		k_sem_give(&wifi_mqtt_location_ready);
	} else {
		if (result) {
			LOG_ERR("Unable to determine location from cellular data, error: %d",
				result->err);
		}
		/* Reset the semaphore to unblock location_service_get_cell_location_nrf_cloud()
		 * and make it return an error.
		 */
		k_sem_reset(&wifi_mqtt_location_ready);
	}
}

int wifi_nrf_cloud_pos_get(char *rcv_buf, size_t rcv_buf_len,
			   const struct location_wifi_serv_pos_req *request,
			   struct location_data *result)
{
	ARG_UNUSED(rcv_buf);
	ARG_UNUSED(rcv_buf_len);
	int err;

	k_sem_reset(&wifi_mqtt_location_ready);

	LOG_DBG("Sending nRF Cloud Wi-Fi positioning request (MQTT)");

	err = nrf_cloud_location_request(
		NULL, request->scanning_results, true, wifi_nrf_cloud_mqtt_location_ready_cb);
	if (err == -EACCES) {
		LOG_ERR("nRF Cloud connection is not established");
		return err;
	} else if (err) {
		LOG_ERR("Failed to request nRF Cloud Wi-Fi positioning data, error: %d", err);
		return err;
	}

	if (k_sem_take(&wifi_mqtt_location_ready, K_MSEC(request->timeout_ms)) == -EAGAIN) {
		LOG_ERR("nRF Cloud Wi-Fi positioning data request timed out or "
			"cloud did not return a location");
		return -ETIMEDOUT;
	}

	*result = nrf_cloud_location;

	return err;
}

#elif defined(CONFIG_NRF_CLOUD_REST)

int wifi_nrf_cloud_pos_get(char *rcv_buf, size_t rcv_buf_len,
			   const struct location_wifi_serv_pos_req *request,
			   struct location_data *result)
{
	int err;
	struct nrf_cloud_location_result nrf_cloud_result;
	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = request->timeout_ms,
		.rx_buf = rcv_buf,
		.rx_buf_len = rcv_buf_len,
		.fragment_size = 0
	};
	const struct nrf_cloud_rest_location_request loc_req = {
		.cell_info = NULL,
		.wifi_info = (struct wifi_scan_info *)request->scanning_results,
	};

	LOG_DBG("Sending nRF Cloud Wi-Fi positioning request (REST)");
	err = nrf_cloud_rest_location_get(&rest_ctx, &loc_req, &nrf_cloud_result);
	if (!err) {
		result->accuracy = (double)nrf_cloud_result.unc;
		result->latitude = nrf_cloud_result.lat;
		result->longitude = nrf_cloud_result.lon;
	}

	return err;
}
#endif
