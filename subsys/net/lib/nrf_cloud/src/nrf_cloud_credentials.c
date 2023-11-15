/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <net/nrf_cloud.h>
#include <zephyr/logging/log.h>
#include <modem/modem_key_mgmt.h>
#include <zephyr/net/tls_credentials.h>

LOG_MODULE_REGISTER(nrf_cloud_credentials, CONFIG_NRF_CLOUD_LOG_LEVEL);

enum nrf_cloud_cred_type {
	NRF_CLOUD_CRED_CA_CERT,
	NRF_CLOUD_CRED_CLIENT_CERT,
	NRF_CLOUD_CRED_PRIVATE_KEY
};

/**
 * @brief Check whether a specified credential exists in any enabled credentials subsystem.
 *
 * @retval 0 if the specified credential exists.
 * @retval -EIO Error while checking if specified credential exists.
 * @retval -ENOTSUP if the specified  credential does not exist.
 */
static int nrf_cloud_credential_check(uint32_t sec_tag, enum nrf_cloud_cred_type cred_type) {
	enum modem_key_mgmt_cred_type modem_type;
	enum tls_credential_type tls_type;
	char *debug_tag;
	int ret;
	bool exists;

	/* Translate universal credential type into credential subsystem types. */
	switch(cred_type) {
	case NRF_CLOUD_CRED_CA_CERT:
		modem_type = MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN;
		tls_type = TLS_CREDENTIAL_CA_CERTIFICATE;
		debug_tag = "CA cert";
		break;
	case NRF_CLOUD_CRED_CLIENT_CERT:
		modem_type = MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT;
		tls_type = TLS_CREDENTIAL_SERVER_CERTIFICATE;
		debug_tag = "client cert";
		break;
	case NRF_CLOUD_CRED_PRIVATE_KEY:
		modem_type = MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT;
		tls_type = TLS_CREDENTIAL_PRIVATE_KEY;
		debug_tag = "private key";
		break;
	default:
		/* Invalid credential type specified. */
		return -EIO;
	}

	/* Check each enabled credentials subsystem for the credential. */

#if defined(CONFIG_MODEM_KEY_MGMT)
	/* Modem credentials subsystem */
	ret = modem_key_mgmt_exists(sec_tag, modem_type, &exists);
	if (ret < 0) {
		LOG_ERR("modem_key_mgmt_exists() failed for %s in sec tag %u, error: %d",
			debug_tag, sec_tag, ret);
		return -EIO;
	}
	if (exists) {
		return 0;
	}
#endif /* CONFIG_MODEM_KEY_MGMT */

#if defined(TLS_CREDENTIALS)
	/* Native credentials subsystem */
	if (credential_get((sec_tag_t)sec_tag, tls_type)) {
		return 0;
	}
#endif /* TLS_CREDENTIALS */

	return -ENOTSUP;
}

int nrf_cloud_credentials_configured_check(void)
{
	int ret;
	struct nrf_cloud_credentials_status cs;

	ret = nrf_cloud_credentials_check(&cs);
	if (ret) {
		LOG_ERR("nrf_cloud_credentials_check failed, error: %d", ret);
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) || IS_ENABLED(CONFIG_NRF_CLOUD_COAP) ||
	    IS_ENABLED(CONFIG_NRF_CLOUD_REST)) {
		if (!cs.ca) {
			LOG_ERR("CA Certificate not found in sec tag %d", cs.sec_tag);
			ret = -ENOTSUP;
		}
		if (!cs.prv_key) {
			LOG_ERR("Private Key not found in sec tag %d", cs.sec_tag);
			ret = -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) || IS_ENABLED(CONFIG_NRF_CLOUD_COAP)) {
		if (!cs.client_cert) {
			LOG_ERR("Client Certificate not found in sec tag %d", cs.sec_tag);
			ret = -ENOTSUP;
		}
	}

	return ret;
}

int nrf_cloud_credentials_check(struct nrf_cloud_credentials_status *const cs)
{
	if (!cs) {
		return -EINVAL;
	}

	int ret;

	memset(cs, 0, sizeof(*cs));

	cs->sec_tag = CONFIG_NRF_CLOUD_SEC_TAG;

#if defined(CONFIG_NRF_CLOUD_COAP_SEC_TAG)
	cs->sec_tag = CONFIG_NRF_CLOUD_COAP_SEC_TAG;
#endif

	/* Check for CA cert */
	ret = nrf_cloud_credential_check(cs->sec_tag, NRF_CLOUD_CRED_CA_CERT);
	if (ret == -EIO) {
		return -EIO;
	}
	cs->ca = (ret == 0);

	/* Check for client cert */
	ret = nrf_cloud_credential_check(cs->sec_tag, NRF_CLOUD_CRED_CLIENT_CERT);
	if (ret == -EIO) {
		return -EIO;
	}
	cs->client_cert = (ret == 0);

	/* Check for private key */
	ret = nrf_cloud_credential_check(cs->sec_tag, NRF_CLOUD_CRED_PRIVATE_KEY);
	if (ret == -EIO) {
		return -EIO;
	}
	cs->prv_key = (ret == 0);

	LOG_DBG("Sec Tag: %u, CA: %s, Client Cert: %s, Private Key: %s",
		cs->sec_tag,
		cs->ca		? "Yes" : "No",
		cs->client_cert	? "Yes" : "No",
		cs->prv_key	? "Yes" : "No");

	return 0;
}
