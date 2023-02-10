/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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

#include "cloud_service.h"
#include "cloud_service_nrf.h"
#include "cloud_service_here_rest.h"
#include "cloud_service_utils.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)

BUILD_ASSERT(IS_ENABLED(CONFIG_LOCATION_SERVICE_NRF_CLOUD) ||
	     IS_ENABLED(CONFIG_LOCATION_SERVICE_HERE),
	     "At least one location service must be enabled");

static char recv_buf[CONFIG_LOCATION_METHOD_CELLULAR_RECV_BUF_SIZE];

int cloud_service_location_get(
	const struct cloud_service_pos_req *params,
	struct location_data *location)
{
	__ASSERT_NO_MSG(params != NULL);
	__ASSERT_NO_MSG(params->cell_data != NULL || params->wifi_data != NULL);
	__ASSERT_NO_MSG(location != NULL);

	LOG_DBG("Cloud service location parameters:");
	LOG_DBG("  Service: %d", params->service);
	LOG_DBG("  Timeout: %dms", params->timeout_ms);

#if defined(CONFIG_LOCATION_SERVICE_NRF_CLOUD)
	if (params->service == LOCATION_SERVICE_NRF_CLOUD ||
	    params->service == LOCATION_SERVICE_ANY) {
		return cloud_service_nrf_pos_get(
			params, recv_buf, sizeof(recv_buf), location);
	}
#endif
#if defined(CONFIG_LOCATION_SERVICE_HERE)
	if (params->service == LOCATION_SERVICE_HERE ||
	    params->service == LOCATION_SERVICE_ANY) {
		return cloud_service_here_rest_pos_get(
			params, recv_buf, sizeof(recv_buf), location);
	}
#endif
	/* We should never get here as at least one service must be enabled */
	return -ENOTSUP;
}

void cloud_service_init(void)
{
#if defined(CONFIG_LOCATION_SERVICE_HERE)
	int ret = cloud_service_utils_provision_ca_certificates();

	if (ret) {
		LOG_ERR("Certificate provisioning failed, ret %d", ret);
		if (ret == -EACCES) {
			LOG_WRN("err: -EACCES, that might indicate that modem is in state where "
				"cert cannot be written, i.e. not in pwroff or offline");
		}
	}
#endif
}

#endif /* !defined(CONFIG_LOCATION_SERVICE_EXTERNAL) */
