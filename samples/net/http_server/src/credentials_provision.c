/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

static const unsigned char server_certificate[] = {
#if defined(HTTP_SERVER_CA_CERT)
#include HTTP_SERVER_CA_CERT

/* Null terminate certificate */
(0x00)
#else
""
#endif
};

static const unsigned char server_private_key[] = {
#if defined(HTTP_SERVER_PRIVATE_KEY)
#include HTTP_SERVER_PRIVATE_KEY

/* Null terminate certificate */
(0x00)
#else
""
#endif
};

LOG_MODULE_REGISTER(http_server_credentials_provision, CONFIG_HTTP_SERVER_SAMPLE_LOG_LEVEL);

static int credential_add_or_replace(sec_tag_t tag, enum tls_credential_type type,
				      const void *cred, size_t credlen, const char *name)
{
	int ret;

	ret = tls_credential_add(tag, type, cred, credlen);
	if (ret == -EEXIST) {
		LOG_DBG("%s already exists, sec tag: %d, replacing", name, tag);

		ret = tls_credential_delete(tag, type);
		if (ret < 0) {
			LOG_ERR("Failed to delete stale %s: %d", name, ret);
			return ret;
		}

		ret = tls_credential_add(tag, type, cred, credlen);
	}

	if (ret < 0) {
		LOG_ERR("Failed to register %s: %d", name, ret);
	}

	return ret;
}

int credentials_provision(void)
{
	int ret;

	ret = credential_add_or_replace(CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG,
					 TLS_CREDENTIAL_CA_CERTIFICATE,
					 server_certificate, sizeof(server_certificate),
					 "CA certificate");
	if (ret < 0) {
		return ret;
	}

	ret = credential_add_or_replace(CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG,
					 TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
					 server_certificate, sizeof(server_certificate),
					 "public certificate");
	if (ret < 0) {
		return ret;
	}

	ret = credential_add_or_replace(CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG,
					 TLS_CREDENTIAL_PRIVATE_KEY,
					 server_private_key, sizeof(server_private_key),
					 "private key");
	if (ret < 0) {
		return ret;
	}

	return 0;
}
