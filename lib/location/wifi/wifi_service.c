/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

#include <modem/location.h>

#include "wifi_service.h"
#if defined(CONFIG_LOCATION_SERVICE_HERE)
#include "wifi_here_rest.h"
#endif
#if defined(CONFIG_LOCATION_SERVICE_NRF_CLOUD)
#include "wifi_nrf_cloud.h"
#endif

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)

BUILD_ASSERT(IS_ENABLED(CONFIG_LOCATION_SERVICE_NRF_CLOUD) ||
	     IS_ENABLED(CONFIG_LOCATION_SERVICE_HERE),
	     "At least one location service must be enabled");

/* Buffer for receiving REST service responses */
static char location_wifi_receive_buffer[CONFIG_LOCATION_METHOD_WIFI_REST_RECV_BUF_SIZE];

int wifi_service_location_get(enum location_service service,
			   const struct location_wifi_serv_pos_req *request,
			   struct location_data *result)
{
#if defined(CONFIG_LOCATION_SERVICE_NRF_CLOUD)
	if (service == LOCATION_SERVICE_NRF_CLOUD || service == LOCATION_SERVICE_ANY) {
		return wifi_nrf_cloud_pos_get(location_wifi_receive_buffer,
					      CONFIG_LOCATION_METHOD_WIFI_REST_RECV_BUF_SIZE,
					      request, result);
	}
#endif
#if defined(CONFIG_LOCATION_SERVICE_HERE)
	if (service == LOCATION_SERVICE_HERE || service == LOCATION_SERVICE_ANY) {
		return wifi_here_rest_pos_get(location_wifi_receive_buffer,
					     CONFIG_LOCATION_METHOD_WIFI_REST_RECV_BUF_SIZE,
					     request, result);
	}
#endif
	LOG_ERR("Requested Wi-Fi positioning service not configured on.");
	return -ENOTSUP;
}
#endif /* !defined(CONFIG_LOCATION_SERVICE_EXTERNAL) */
