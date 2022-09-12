/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MULTICELL_LOCATION_H_
#define MULTICELL_LOCATION_H_

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup multicell_location Multicell location
 * @{
 */

struct multicell_location {
	double latitude;
	double longitude;
	float accuracy;
};

/** Cellular positioning service. */
enum multicell_service {
	/**
	 * @brief Use any location service that has been configured to be available.
	 *
	 * @details This is especially useful when only one service is configured but can be used
	 * even if many are available, in which case selected service is undefined.
	 */
	MULTICELL_SERVICE_ANY,
	/** nRF Cloud location service. */
	MULTICELL_SERVICE_NRF_CLOUD,
	/** Here location service. */
	MULTICELL_SERVICE_HERE
};

/** Cellular positioning input parameters. */
struct multicell_location_params {
	/** Cellular positioning service to be used. */
	enum multicell_service service;
	/** Neighbor cell data. */
	const struct lte_lc_cells_info *cell_data;
	/**
	 * @brief Timeout (in milliseconds) of how long the cellular positioning procedure can take.
	 * SYS_FOREVER_MS means that the timer is disabled.
	 */
	int32_t timeout;
};

/**
 * @brief Send a request for location based on cell measurements to the
 *        selected location service.
 *
 * @note This function will block the calling thread until a response
 *       is received from the location service, or timeout has elapsed.
 *
 * @note Certificate must be provisioned before a request can be sent,
 *       @ref multicell_location_provision_certificate.
 *
 * @param[in] params Cellular positioning parameters to be used.
 * @param[out] location Location.
 *
 * @return 0 on success, or negative error code on failure.
 * @retval -ENOENT No cellular cells found from cell_data. I.e., even current cell
 *                 is not available and there is no cellular connectivity.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOMSG Parsing response from the location service failed.
 */
int multicell_location_get(const struct multicell_location_params *params,
			   struct multicell_location *location);

/**
 * @brief Provision TLS certificate that the selected location service requires
 *	  for HTTPS connections.
 *	  Certificate provisioning must be done before location requests can
 *	  successfully be executed, either using this API or some other method.
 *
 * @note Certificate provisioning must happen when LTE is not active in the modem.
 *	 This is typically achieved by calling this API prior to setting up the
 *	 LTE link in an application, but it can also be done by setting the
 *	 modem in offline mode before provisioning.
 *
 * @param[in] overwrite If this flag is set, any CA certificate currently
 *		        provisioned to CONFIG_MULTICELL_LOCATION_TLS_SEC_TAG is
 *		        overwritten.
 *
 * @return 0 on success, or negative error code on failure.
 */
int multicell_location_provision_certificate(bool overwrite);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* MULTICELL_LOCATION_H_ */
