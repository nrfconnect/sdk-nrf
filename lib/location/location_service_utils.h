/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOCATION_SERVICE_UTILS_H
#define LOCATION_SERVICE_UTILS_H

/**
 * @brief Provision TLS certificate that the selected location service requires
 *	  for HTTPS connections.
 *	  The certificate must be provisioned before location requests can
 *	  be executed successfully, either using this API or some other method.
 *
 * @note The certificate must be provisioned when LTE is not active in the modem.
 *	 Call this API before setting up the
 *	 LTE link in an application, or set the
 *	 modem in offline mode before provisioning.
 *
 * @return 0 on success, or negative error code on failure.
 */
int location_service_utils_provision_ca_certificates(void);

#endif /* LOCATION_SERVICE_UTILS_H */
