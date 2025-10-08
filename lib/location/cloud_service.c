/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_NRF_CLOUD_MQTT)
#include <net/nrf_cloud_location.h>
#elif defined(CONFIG_NRF_CLOUD_REST)
#include <net/nrf_cloud_rest.h>
#elif defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#endif

#include "cloud_service.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

/* Verify that MQTT, REST or CoAP is enabled */
BUILD_ASSERT(
	IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) ||
	IS_ENABLED(CONFIG_NRF_CLOUD_REST) ||
	IS_ENABLED(CONFIG_NRF_CLOUD_COAP),
	"CONFIG_NRF_CLOUD_MQTT, CONFIG_NRF_CLOUD_REST or CONFIG_NRF_CLOUD_COAP transport "
	"must be enabled");

static char recv_buf[CONFIG_LOCATION_SERVICE_CLOUD_RECV_BUF_SIZE];

#if defined(CONFIG_NRF_CLOUD_MQTT)
/* Verify that LOCATION is enabled if MQTT is enabled */
BUILD_ASSERT(
	IS_ENABLED(CONFIG_NRF_CLOUD_LOCATION),
	"If CONFIG_NRF_CLOUD_MQTT is enabled also CONFIG_NRF_CLOUD_LOCATION must be enabled");

static struct location_data cloud_location;
static K_SEM_DEFINE(location_ready, 0, 1);
#endif

#if defined(CONFIG_NRF_CLOUD_MQTT)
static void cloud_service_location_ready_cb(
	const struct nrf_cloud_location_result *const result)
{
	if ((result != NULL) && (result->err == NRF_CLOUD_ERROR_NONE)) {
		cloud_location.latitude = result->lat;
		cloud_location.longitude = result->lon;
		cloud_location.accuracy = (double)result->unc;

		k_sem_give(&location_ready);
	} else {
		if (result) {
			LOG_ERR("Unable to determine location from cloud service, error: %d",
				result->err);
		}
		/* Reset the semaphore to unblock cloud_service_pos_get()
		 * and make it return an error.
		 */
		k_sem_reset(&location_ready);
	}
}

static int cloud_service_pos_get(
	const struct cloud_service_pos_req *params,
	char * const rcv_buf,
	const size_t rcv_buf_len,
	struct location_data *const location)
{
	ARG_UNUSED(rcv_buf);
	ARG_UNUSED(rcv_buf_len);

	int err;

	k_sem_reset(&location_ready);

	LOG_DBG("Sending positioning request (MQTT)");
	err = nrf_cloud_location_request_timestamped(
		params->cell_data, params->wifi_data, NULL, cloud_service_location_ready_cb, 1759934279392);
	if (err == -EACCES) {
		LOG_ERR("Cloud connection is not established");
		return err;
	} else if (err) {
		LOG_ERR("Failed to request positioning data, error: %d", err);
		return err;
	}

	LOG_DBG("Positioning request sent");

	if (k_sem_take(&location_ready, K_MSEC(params->timeout_ms)) == -EAGAIN) {
		LOG_ERR("Positioning data request timed out or "
			"cloud did not return a location");
		return -ETIMEDOUT;
	}

	*location = cloud_location;

	return err;
}
#else /* defined(CONFIG_NRF_CLOUD_MQTT) */
static int cloud_service_pos_get(
	const struct cloud_service_pos_req *params,
	char * const rcv_buf,
	const size_t rcv_buf_len,
	struct location_data *const location)
{
	int err;
#if defined(CONFIG_NRF_CLOUD_REST)
	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = params->timeout_ms,
		.rx_buf = rcv_buf,
		.rx_buf_len = rcv_buf_len,
		.fragment_size = 0
	};
#endif
	const struct nrf_cloud_rest_location_request loc_req = {
		.cell_info = params->cell_data,
		.wifi_info = params->wifi_data,
		.config = NULL
	};
	struct nrf_cloud_location_result result;

#if defined(CONFIG_NRF_CLOUD_REST)
	LOG_DBG("Sending positioning request (REST)");
	err = nrf_cloud_rest_location_get(&rest_ctx, &loc_req, &result);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	LOG_DBG("Sending positioning request (CoAP)");
	err = nrf_cloud_coap_location_get(&loc_req, &result);
#endif
	if (!err) {
		location->accuracy = (double)result.unc;
		location->latitude = result.lat;
		location->longitude = result.lon;
	}

	return err;
}
#endif /* defined(CONFIG_NRF_CLOUD_MQTT) */

int cloud_service_location_get(
	const struct cloud_service_pos_req *params,
	struct location_data *location)
{
	__ASSERT_NO_MSG(params != NULL);
	__ASSERT_NO_MSG(params->cell_data != NULL || params->wifi_data != NULL);
	__ASSERT_NO_MSG(location != NULL);

	LOG_DBG("Cloud service location parameters:");
	LOG_DBG("  Timeout: %dms", params->timeout_ms);

	return cloud_service_pos_get(
		params, recv_buf, sizeof(recv_buf), location);
}
