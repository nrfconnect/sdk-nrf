/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

/* Include the server certificate and private key. These are intended for testing and should
 * not be used for anything else.
 */
#include "server_certificate.h"
#include "server_private_key.h"

LOG_MODULE_REGISTER(http_server_credentials_provision, CONFIG_HTTP_SERVER_SAMPLE_LOG_LEVEL);

int credentials_provision(void)
{
	int ret;

	ret = tls_credential_add(CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 server_certificate,
				 sizeof(server_certificate));

	if (ret == -EEXIST) {
		LOG_DBG("CA certificate already exists, sec tag: %d",
			CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG);
	} else if (ret < 0) {
		LOG_ERR("Failed to register CA certificate: %d", ret);
		return ret;
	}

	ret = tls_credential_add(CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG,
				 TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 server_certificate,
				 sizeof(server_certificate));
	if (ret == -EEXIST) {
		LOG_DBG("Public certificate already exists, sec tag: %d",
			CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG);
	} else if (ret < 0) {
		LOG_ERR("Failed to register public certificate: %d", ret);
		return ret;
	}

	ret = tls_credential_add(CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 server_private_key, sizeof(server_private_key));

	if (ret == -EEXIST) {
		LOG_DBG("Private key already exists, sec tag: %d",
			CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG);
	} else if (ret < 0) {
		LOG_ERR("Failed to register private key: %d", ret);
		return ret;
	}

	return 0;
}
