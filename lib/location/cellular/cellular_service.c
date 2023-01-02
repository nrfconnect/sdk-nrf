/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <assert.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/tls_credentials.h>
#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>

#include "cellular_service.h"
#include "cellular_nrf_cloud.h"
#include "cellular_here_rest.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)

BUILD_ASSERT(IS_ENABLED(CONFIG_LOCATION_SERVICE_NRF_CLOUD) ||
	     IS_ENABLED(CONFIG_LOCATION_SERVICE_HERE),
	     "At least one location service must be enabled");

static char recv_buf[CONFIG_LOCATION_METHOD_CELLULAR_RECV_BUF_SIZE];

int cellular_service_location_get(
	const struct location_cellular_serv_pos_req *params,
	struct location_data *location)
{
	if (params == NULL || (params->cell_data == NULL) || (location == NULL)) {
		return -EINVAL;
	}

	LOG_DBG("Cellular service location parameters:");
	LOG_DBG("  Service: %d", params->service);
	LOG_DBG("  Timeout: %dms", params->timeout);

	if (params->cell_data->current_cell.id == LTE_LC_CELL_EUTRAN_ID_INVALID) {
		LOG_WRN("Invalid cell ID, device may not be connected to a network");
		return -ENOENT;
	}

#if defined(CONFIG_LOCATION_SERVICE_NRF_CLOUD)
	if (params->service == LOCATION_SERVICE_NRF_CLOUD ||
	    params->service == LOCATION_SERVICE_ANY) {
		return cellular_nrf_cloud_pos_get(
			params, recv_buf, sizeof(recv_buf), location);
	}
#endif
#if defined(CONFIG_LOCATION_SERVICE_HERE)
	if (params->service == LOCATION_SERVICE_HERE ||
	    params->service == LOCATION_SERVICE_ANY) {
		return cellular_here_rest_pos_get(
			params, recv_buf, sizeof(recv_buf), location);
	}
#endif
	/* We should never get here as at least one service must be enabled */
	return -ENOTSUP;
}

#endif /* !defined(CONFIG_LOCATION_SERVICE_EXTERNAL) */
