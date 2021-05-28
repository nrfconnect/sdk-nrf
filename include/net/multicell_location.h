/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MULTICELL_LOCATION_H_
#define MULTICELL_LOCATION_H_

#include <zephyr.h>
#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup multicell_location Multicell location
 * @{
 */

struct multicell_location {
	float latitude;
	float longitude;
	float accuracy;
};

/* @brief Send a request for location based on cell measurements to the
 *        selected location service.
 *
 * @note This function will block until a response
 *       is received from the location service.
 *
 * @note Certificate must be provisioned before a request can be sent,
 *       @ref multicell_location_provision_certificate.
 *
 * @param cell_data Pointer to neighbor cell data.
 * @param location Pointer to location.
 *
 * @return 0 on success, or negative error code on failure.
 */
int multicell_location_get(const struct lte_lc_cells_info *cell_data,
			   struct multicell_location *location);

/* @brief Provision TLS certificate that the selected location service requires
 *	  for HTTPS connections.
 *	  Certificate provisioning must be done before location requests can
 *	  successfully be executed, either using this API or some other method.
 *
 * @note Certificate provisioning must happen when LTE is not active in the modem.
 *	 This is typically achieved by calling this API prior to setting up the
 *	 LTE link in an application, but it can also be done by setting the
 *	 modem in offline mode before provisioning.
 *
 * @param overwrite If this flag is set, any CA certificate currently
 *		    provisioned to CONFIG_MULTICELL_LOCATION_TLS_SEC_TAG is
 *		    overwritten.
 *
 * @return 0 on success, or negative error code on failure.
 */
int multicell_location_provision_certificate(bool overwrite);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* MULTICELL_LOCATION_H_ */
