/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#if defined(CONFIG_POSIX_API)
#include <posix/arpa/inet.h>
#include <posix/unistd.h>
#include <posix/netdb.h>
#include <posix/sys/socket.h>
#else
#include <net/socket.h>
#endif
#include <modem/lte_lc.h>
#include <net/tls_credentials.h>
#include <modem/modem_key_mgmt.h>
#include <net/multicell_location.h>

#include "location_service.h"

#include <logging/log.h>

#define TLS_SEC_TAG	CONFIG_MULTICELL_LOCATION_TLS_SEC_TAG

LOG_MODULE_REGISTER(multicell_location, CONFIG_MULTICELL_LOCATION_LOG_LEVEL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_MULTICELL_LOCATION_SERVICE_NONE),
	     "A location service must be enabled");

static char recv_buf[CONFIG_MULTICELL_LOCATION_RECV_BUF_SIZE];


int multicell_location_get(const struct lte_lc_cells_info *cell_data,
			   const char * const device_id,
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

	return location_service_get_cell_location(
		cell_data, device_id, recv_buf, sizeof(recv_buf), location);
}

int multicell_location_provision_certificate(bool overwrite)
{
	int err;
	bool exists;
	const char *certificate = location_service_get_certificate();

	if (certificate == NULL) {
		LOG_ERR("No certificate was provided by the location service");
		return -EFAULT;
	}

	err = modem_key_mgmt_exists(TLS_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    &exists);
	if (err) {
		LOG_ERR("Failed to check for certificates err %d", err);
		return err;
	}

	if (exists && overwrite) {
		/* For the sake of simplicity we delete what is provisioned
		 * with our security tag and reprovision our certificate.
		 */
		err = modem_key_mgmt_delete(TLS_SEC_TAG,
					    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			LOG_ERR("Failed to delete existing certificate, err %d", err);
		}
	} else if (exists && !overwrite) {
		LOG_INF("A certificate is already provisioned to sec tag %d",
			TLS_SEC_TAG);
		return 0;
	}

	LOG_INF("Provisioning certificate");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   certificate, strlen(certificate));
	if (err) {
		LOG_ERR("Failed to provision certificate, err %d", err);
		return err;
	}

	return 0;
}
