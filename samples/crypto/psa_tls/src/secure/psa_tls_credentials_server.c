/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psa_tls_credentials_server_secure);

#include <nrf.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/net/tls_credentials.h>
#include "psa_tls_credentials.h"
#include "certificate.h"


/** @brief Function for registering the server certificate and
 * server private key for the TLS handshake.
 */
int tls_set_credentials(void)
{
	LOG_INF("Registering Server certificate and key");

	int err = tls_credential_add(SERVER_CERTIFICATE_TAG,
				     TLS_CREDENTIAL_SERVER_CERTIFICATE,
				     server_certificate,
				     sizeof(server_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d", err);
		return err;
	}

	err = tls_credential_add(SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 private_key, sizeof(private_key));
	if (err < 0) {
		LOG_ERR("Failed to register private key: %d", err);
		return err;
	}

	return APP_SUCCESS;
}
