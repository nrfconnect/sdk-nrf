/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud_pgps.h>
#include "nrf_cloud_pgps_internal.h"
#include "nrf_cloud_fsm.h"

int nrf_cloud_pgps_request(const struct gps_pgps_request *request)
{
	if (nfsm_get_current_state() != STATE_DC_CONNECTED) {
		return -EACCES;
	}

	return nrf_cloud_pgps_request_internal(request);
}

int nrf_cloud_pgps_request_all(void)
{
	return nrf_cloud_pgps_request_internal_all();
}
