/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOCATION_SERVICE_H_
#define LOCATION_SERVICE_H_

#include <zephyr.h>
#include <modem/lte_lc.h>
#include <net/multicell_location.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Generate an HTTPS request in the format the location service expects.
 *
 * @param cell_data Pointer to neighbor cell data.
 * @param buf Buffer for storing the HTTP request.
 * @param buf_len Length of the provided buffer.
 *
 * @return 0 on success, or negative error code on failure.
 */
int location_service_generate_request(const struct lte_lc_cells_info *cell_data,
				      char *buf, size_t buf_len);

/* @brief Get pointer to the location service's null-terminated hostname.
 *
 * @return A pointer to null-terminated hostname in case of success,
 *	   or NULL in case of failure.
 */
const char *location_service_get_hostname(void);

/* @brief Get pointer to certificate to location service.
 *
 * @return A pointer to null-terminated root CA certificate in case of success,
 *	   or NULL in case of failure.
 */
const char *location_service_get_certificate(void);

/* @brief Parse a response from a location service, and populate the provided
 *	  location structure.
 *
 * @param response Response from location service.
 * @param location Storage for the result from response parsing.
 *
 * @return 0 on success, or -1 on failure.
 */
int location_service_parse_response(const char *response, struct multicell_location *location);

#ifdef __cplusplus
}
#endif

#endif /* LOCATION_SERVICE_H_ */
