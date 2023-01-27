/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CLOUD_SERVICE_H_
#define CLOUD_SERVICE_H_

#include <modem/location.h>
#include <modem/lte_lc.h>
#include <net/wifi_location_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Cloud positioning input parameters. */
struct cloud_service_pos_req {
	/** Cloud positioning service to be used. */
	enum location_service service;
	/** Neighbor cell data. */
	struct lte_lc_cells_info *cell_data;
	/** Wi-Fi scanning results data. */
	struct wifi_scan_info *wifi_data;
	/**
	 * @brief Timeout (in milliseconds) of the cloud positioning procedure.
	 * SYS_FOREVER_MS means that the timer is disabled.
	 */
	int32_t timeout_ms;
};

/**
 * @brief Generate location request, send, and parse response.
 *
 * @param[in] params Cloud positioning parameters to be used.
 * @param[out] location Storage for the result from response parsing.
 *
 * @return 0 on success, or negative error code on failure.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOMSG Parsing response from the location service failed.
 */
int cloud_service_location_get(
	const struct cloud_service_pos_req *params,
	struct location_data *location);

#ifdef __cplusplus
}
#endif

#endif /* CLOUD_SERVICE_H_ */
