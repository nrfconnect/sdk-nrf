/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <assert.h>
#include <net/multicell_location.h>

#include "location_service.h"
#include "nrf_cloud_integration.h"
#include "here_integration.h"

static char recv_buf[CONFIG_MULTICELL_LOCATION_RECV_BUF_SIZE];

const char *location_service_get_certificate(enum multicell_service service)
{
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD)
	if (service == MULTICELL_SERVICE_NRF_CLOUD) {
		return location_service_get_certificate_nrf_cloud();
	}
#endif
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_HERE)
	if (service == MULTICELL_SERVICE_HERE) {
		return location_service_get_certificate_here();
	}
#endif
	return NULL;
}

int location_service_get_cell_location(
	const struct multicell_location_params *params,
	struct multicell_location *location)
{
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD)
	if (params->service == MULTICELL_SERVICE_NRF_CLOUD ||
	    params->service == MULTICELL_SERVICE_ANY) {
		return location_service_get_cell_location_nrf_cloud(
			params, recv_buf, sizeof(recv_buf), location);
	}
#endif
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_HERE)
	if (params->service == MULTICELL_SERVICE_HERE ||
	    params->service == MULTICELL_SERVICE_ANY) {
		return location_service_get_cell_location_here(
			params, recv_buf, sizeof(recv_buf), location);
	}
#endif
	/* We should never get here as at least one service must be enabled */
	return -ENOTSUP;
}
