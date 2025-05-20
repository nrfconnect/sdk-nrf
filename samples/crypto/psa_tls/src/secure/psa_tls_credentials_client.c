/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psa_tls_credentials_client_secure);

#include <nrf.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/net/tls_credentials.h>
#include "psa_tls_credentials.h"
#include "certificate.h"


/** @brief Function for registering the CA certificate for the TLS handshake.
 */
int tls_set_credentials(void)
{
	LOG_INF("Registering Client CA certificate");

	int err = tls_credential_add(CA_CERTIFICATE_TAG,
				     TLS_CREDENTIAL_CA_CERTIFICATE,
				     ca_certificate, sizeof(ca_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register CA certificate: %d", err);
		return err;
	}

	return APP_SUCCESS;
}
