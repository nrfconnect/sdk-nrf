/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOCATION_SERVICE_H_
#define LOCATION_SERVICE_H_

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <net/multicell_location.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get pointer to certificate to location service.
 *
 * @param[in] service Cellular positioning service to be used.
 *
 * @return A pointer to null-terminated root CA certificate in case of success,
 *         or NULL in case of failure.
 */
const char *location_service_get_certificate(enum multicell_service service);

/**
 * @brief Generate location request, send, and parse response.
 *
 * @param[in] params Cellular positioning parameters to be used.
 * @param[out] location Storage for the result from response parsing.
 *
 * @return 0 on success, or negative error code on failure.
 * @retval -ENOENT No cellular cells found from cell_data. I.e., even current cell
 *                 is not available and there is no cellular connectivity.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOMSG Parsing response from the location service failed.
 */
int location_service_get_cell_location(
	const struct multicell_location_params *params,
	struct multicell_location *location);

#ifdef __cplusplus
}
#endif

#endif /* LOCATION_SERVICE_H_ */
