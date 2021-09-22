/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <assert.h>

#include <logging/log.h>

#include <modem/location.h>

#if defined(CONFIG_LOCATION_METHOD_WLAN_SERVICE_HERE)
#include "rest_here_wlan.h"
#endif
#if defined(CONFIG_LOCATION_METHOD_WLAN_SERVICE_SKYHOOK)
#include "rest_skyhook_wlan.h"
#endif
#if defined(CONFIG_LOCATION_METHOD_WLAN_SERVICE_NRF_CLOUD)
#include "rest_nrf_cloud_wlan.h"
#endif

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

/* Buffer for receiving REST service responses */
static char loc_wlan_receive_buffer[CONFIG_LOCATION_METHOD_WLAN_REST_RECV_BUF_SIZE];

int rest_services_wlan_location_get(enum loc_wlan_service service,
			   const struct rest_wlan_pos_request *request,
			   struct rest_wlan_pos_result *result)
{
	int ret = -EINVAL;

#if defined(CONFIG_LOCATION_METHOD_WLAN_SERVICE_HERE)
	if (service == LOC_WLAN_SERVICE_HERE) {
		ret = here_rest_wlan_pos_get(loc_wlan_receive_buffer,
					     CONFIG_LOCATION_METHOD_WLAN_REST_RECV_BUF_SIZE,
					     request, result);
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_WLAN_SERVICE_SKYHOOK)
	if (service == LOC_WLAN_SERVICE_SKYHOOK) {
		ret = skyhook_rest_wlan_pos_get(loc_wlan_receive_buffer,
						CONFIG_LOCATION_METHOD_WLAN_REST_RECV_BUF_SIZE,
						request, result);
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_WLAN_SERVICE_NRF_CLOUD)
	if (service == LOC_WLAN_SERVICE_NRF_CLOUD) {
		ret = nrf_cloud_rest_wlan_pos_get(loc_wlan_receive_buffer,
						CONFIG_LOCATION_METHOD_WLAN_REST_RECV_BUF_SIZE,
						request, result);
	}
#endif
	return ret;
}
