/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>

#include <logging/log.h>

#include <modem/location.h>

#if defined(CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE)
#include "wifi_here_rest.h"
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK)
#include "wifi_skyhook_rest.h"
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD)
#include "wifi_nrf_cloud_rest.h"
#endif

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

/* Buffer for receiving REST service responses */
static char location_wifi_receive_buffer[CONFIG_LOCATION_METHOD_WIFI_REST_RECV_BUF_SIZE];

int rest_services_wifi_location_get(enum location_service service,
			   const struct rest_wifi_pos_request *request,
			   struct location_data *result)
{
#if defined(CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD)
	if (service == LOCATION_SERVICE_NRF_CLOUD || service == LOCATION_SERVICE_ANY) {
		return wifi_nrf_cloud_rest_pos_get(location_wifi_receive_buffer,
						CONFIG_LOCATION_METHOD_WIFI_REST_RECV_BUF_SIZE,
						request, result);
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE)
	if (service == LOCATION_SERVICE_HERE || service == LOCATION_SERVICE_ANY) {
		return wifi_here_rest_pos_get(location_wifi_receive_buffer,
					     CONFIG_LOCATION_METHOD_WIFI_REST_RECV_BUF_SIZE,
					     request, result);
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK)
	if (service == LOCATION_SERVICE_SKYHOOK || service == LOCATION_SERVICE_ANY) {
		return wifi_skyhook_rest_pos_get(location_wifi_receive_buffer,
						CONFIG_LOCATION_METHOD_WIFI_REST_RECV_BUF_SIZE,
						request, result);
	}
#endif
	LOG_ERR("Requested Wi-Fi positioning service not configured on.");
	return -ENOTSUP;
}
