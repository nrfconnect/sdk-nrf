/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LOCATION_SERVICE_H
#define MOSH_LOCATION_SERVICE_H

#if defined(CONFIG_NRF_CLOUD_AGPS)
#include <nrf_modem_gnss.h>
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
#include <modem/lte_lc.h>
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
#include <net/wifi_location_common.h>
#endif

#if defined(CONFIG_NRF_CLOUD_AGPS)
void location_srv_ext_agps_handle(const struct nrf_modem_gnss_agnss_data_frame *agps_req);
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
void location_srv_ext_pgps_handle(const struct gps_pgps_request *pgps_req);
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR) || defined(CONFIG_LOCATION_METHOD_WIFI)
void location_srv_ext_cloud_location_handle(
	const struct location_data_cloud *cloud_location_request,
	bool cloud_resp_enabled);
#endif

#endif /* MOSH_LOCATION_SERVICE_H */
