/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <assert.h>
#include <net/multicell_location.h>

#include "location_service.h"
#include "nrf_cloud_integration.h"
#include "here_integration.h"
#include "skyhook_integration.h"
#include "polte_integration.h"

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
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_SKYHOOK)
	if (service == MULTICELL_SERVICE_SKYHOOK) {
		return location_service_get_certificate_skyhook();
	}
#endif
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_POLTE)
	if (service == MULTICELL_SERVICE_POLTE) {
		return location_service_get_certificate_polte();
	}
#endif
	return NULL;
}

int location_service_get_cell_location(
	enum multicell_service service,
	const struct lte_lc_cells_info *cell_data,
	struct multicell_location *const location)
{
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD)
	if (service == MULTICELL_SERVICE_NRF_CLOUD || service == MULTICELL_SERVICE_ANY) {
		return location_service_get_cell_location_nrf_cloud(
			cell_data, recv_buf, sizeof(recv_buf), location);
	}
#endif
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_HERE)
	if (service == MULTICELL_SERVICE_HERE || service == MULTICELL_SERVICE_ANY) {
		return location_service_get_cell_location_here(
			cell_data, recv_buf, sizeof(recv_buf), location);
	}
#endif
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_SKYHOOK)
	if (service == MULTICELL_SERVICE_SKYHOOK || service == MULTICELL_SERVICE_ANY) {
		return location_service_get_cell_location_skyhook(
			cell_data, recv_buf, sizeof(recv_buf), location);
	}
#endif
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_POLTE)
	if (service == MULTICELL_SERVICE_POLTE || service == MULTICELL_SERVICE_ANY) {
		return location_service_get_cell_location_polte(
			cell_data, recv_buf, sizeof(recv_buf), location);
	}
#endif
	/* We should never get here as at least one service must be enabled */
	return -ENOTSUP;
}
