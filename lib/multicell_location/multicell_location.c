/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <net/socket.h>
#include <modem/lte_lc.h>
#include <net/tls_credentials.h>
#include <modem/modem_key_mgmt.h>
#include <net/multicell_location.h>

#include "location_service.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(multicell_location, CONFIG_MULTICELL_LOCATION_LOG_LEVEL);

BUILD_ASSERT(IS_ENABLED(CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD) ||
	     IS_ENABLED(CONFIG_MULTICELL_LOCATION_SERVICE_HERE) ||
	     IS_ENABLED(CONFIG_MULTICELL_LOCATION_SERVICE_SKYHOOK) ||
	     IS_ENABLED(CONFIG_MULTICELL_LOCATION_SERVICE_POLTE),
	     "At least one location service must be enabled");


int multicell_location_get(enum multicell_service service,
			   const struct lte_lc_cells_info *cell_data,
			   struct multicell_location *location)
{
	if ((cell_data == NULL) || (location == NULL)) {
		return -EINVAL;
	}

	if (cell_data->current_cell.id == LTE_LC_CELL_EUTRAN_ID_INVALID) {
		LOG_WRN("Invalid cell ID, device may not be connected to a network");
		return -ENOENT;
	}

	if (cell_data->ncells_count > CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS) {
		LOG_WRN("Found %d neighbor cells, but %d cells will be used in location request",
			cell_data->ncells_count, CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS);
		LOG_WRN("Increase CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS to use more cells");
	}

	return location_service_get_cell_location(service, cell_data, location);
}

static int multicell_location_provision_service_certificate(
	int sec_tag,
	const char *certificate,
	bool overwrite)
{
	int err;
	bool exists;

	if (certificate == NULL) {
		LOG_ERR("No certificate was provided by the location service");
		return -EFAULT;
	}

	err = modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (err) {
		LOG_ERR("Failed to check for certificates err %d", err);
		return err;
	}

	if (exists && overwrite) {
		/* For the sake of simplicity we delete what is provisioned
		 * with our security tag and reprovision our certificate.
		 */
		err = modem_key_mgmt_delete(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			LOG_ERR("Failed to delete existing certificate, err %d", err);
		}
	} else if (exists && !overwrite) {
		LOG_DBG("A certificate is already provisioned to sec tag %d", sec_tag);
		return 0;
	}

	LOG_INF("Provisioning certificate");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(sec_tag,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   certificate,
				   strlen(certificate));
	if (err) {
		LOG_ERR("Failed to provision certificate, err %d", err);
		return err;
	}

	return 0;
}

int multicell_location_provision_certificate(bool overwrite)
{
	int ret = -ENOTSUP;

#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD)
	ret = multicell_location_provision_service_certificate(
		CONFIG_NRF_CLOUD_SEC_TAG,
		location_service_get_certificate(MULTICELL_SERVICE_NRF_CLOUD),
		overwrite);
	if (ret) {
		return ret;
	}
#endif
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_HERE)
	ret = multicell_location_provision_service_certificate(
		CONFIG_MULTICELL_LOCATION_HERE_TLS_SEC_TAG,
		location_service_get_certificate(MULTICELL_SERVICE_HERE),
		overwrite);
	if (ret) {
		return ret;
	}
#endif
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_SKYHOOK)
	ret = multicell_location_provision_service_certificate(
		CONFIG_MULTICELL_LOCATION_SKYHOOK_TLS_SEC_TAG,
		location_service_get_certificate(MULTICELL_SERVICE_SKYHOOK),
		overwrite);
	if (ret) {
		return ret;
	}
#endif
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_POLTE)
	ret = multicell_location_provision_service_certificate(
		CONFIG_MULTICELL_LOCATION_POLTE_TLS_SEC_TAG,
		location_service_get_certificate(MULTICELL_SERVICE_POLTE),
		overwrite);
	if (ret) {
		return ret;
	}
#endif
	return ret;
}
