/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CELLULAR_SERVICE_H_
#define CELLULAR_SERVICE_H_

#include <modem/location.h>
#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Cellular positioning input parameters. */
struct location_cellular_serv_pos_req {
	/** Cellular positioning service to be used. */
	enum location_service service;
	/** Neighbor cell data. */
	const struct lte_lc_cells_info *cell_data;
	/**
	 * @brief Timeout (in milliseconds) of the cellular positioning procedure.
	 * SYS_FOREVER_MS means that the timer is disabled.
	 */
	int32_t timeout;
};

/**
 * @brief Generate location request, send, and parse response.
 *
 * @param[in] params Cellular positioning parameters to be used.
 * @param[out] location Storage for the result from response parsing.
 *
 * @return 0 on success, or negative error code on failure.
 * @retval -ENOENT No cellular cells found from cell_data. Meaning that even the current cell
 *                 is not available and there is no cellular connectivity.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOMSG Parsing response from the location service failed.
 */
int cellular_service_location_get(
	const struct location_cellular_serv_pos_req *params,
	struct location_data *location);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_H_ */
