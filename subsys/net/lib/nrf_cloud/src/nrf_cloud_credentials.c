/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <net/nrf_cloud.h>
#include <zephyr/logging/log.h>
#include "nrf_cloud_credentials.h"

#if defined(CONFIG_MODEM_KEY_MGMT)
#include <modem/modem_key_mgmt.h>
#endif

LOG_MODULE_REGISTER(nrf_cloud_credentials, CONFIG_NRF_CLOUD_LOG_LEVEL);

enum nrf_cloud_cred_type {
	CA_CERT = 0,
	CLIENT_CERT = 1,
	PRIVATE_KEY = 2,
	CRED_COUNT
};

static int cred_type[CRED_COUNT] = {
#if defined(CONFIG_NRF_CLOUD_CREDENTIALS_MGMT_MODEM)
	MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
	MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
	MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT
#else
	TLS_CREDENTIAL_CA_CERTIFICATE,
	TLS_CREDENTIAL_SERVER_CERTIFICATE,
	TLS_CREDENTIAL_PRIVATE_KEY
#endif
};

#if defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)
#include CONFIG_NRF_CLOUD_CERTIFICATES_FILE

BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_CLOUD_PROVISION_CA_CERT) ||
	     IS_ENABLED(CONFIG_NRF_CLOUD_PROVISION_CLIENT_CERT) ||
	     IS_ENABLED(CONFIG_NRF_CLOUD_PROVISION_PRV_KEY),
	     "No credentials are enabled for provisioning");

static int delete_cred(uint32_t sec_tag, int type)
{
	int ret;

#if defined(CONFIG_NRF_CLOUD_CREDENTIALS_MGMT_MODEM)
	ret = modem_key_mgmt_delete(sec_tag, (enum modem_key_mgmt_cred_type)cred_type[type]);
#else
	ret = tls_credential_delete(sec_tag, (enum tls_credential_type)cred_type[type]);
#endif
	LOG_DBG("Delete credential type %d, result: %d", cred_type[type], ret);
	return ret;
}

static int add_cred(uint32_t sec_tag, int type, const void *cred, const size_t cred_len)
{
	int ret;

#if defined(CONFIG_NRF_CLOUD_CREDENTIALS_MGMT_MODEM)
	ret = modem_key_mgmt_write(sec_tag, (enum modem_key_mgmt_cred_type)cred_type[type],
				   cred, cred_len);
#else
	ret = tls_credential_add(sec_tag, (enum tls_credential_type)cred_type[type],
				 cred, cred_len + 1); /* Add one to include NULL */
#endif
	LOG_DBG("Add credential type %d, result: %d", cred_type[type], ret);
	return ret;
}

int nrf_cloud_credentials_provision(void)
{
	int err = 0;
	uint32_t sec_tag = CONFIG_NRF_CLOUD_SEC_TAG;

#if defined(CONFIG_NRF_CLOUD_COAP)
	sec_tag = CONFIG_NRF_CLOUD_COAP_SEC_TAG;
#endif

	LOG_WRN("CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES is not secure and should be used only for "
		"testing purposes");

	LOG_DBG("Updating credentials in sec tag %u", sec_tag);

#if defined(CONFIG_NRF_CLOUD_PROVISION_CA_CERT)
	(void)delete_cred(sec_tag, CA_CERT);

	err = add_cred(sec_tag, CA_CERT, ca_certificate, strlen(ca_certificate));

	if (err) {
		LOG_ERR("Failed to install CA cert, error: %d", err);
		return err;
	}
#endif

#if defined(CONFIG_NRF_CLOUD_PROVISION_CLIENT_CERT)
	(void)delete_cred(sec_tag, CLIENT_CERT);

	err = add_cred(sec_tag, CLIENT_CERT, device_certificate, strlen(device_certificate));

	if (err) {
		LOG_ERR("Failed to install client cert, error: %d", err);
		return err;
	}
#endif

#if defined(CONFIG_NRF_CLOUD_PROVISION_PRV_KEY)
	(void)delete_cred(sec_tag, PRIVATE_KEY);

	err = add_cred(sec_tag, PRIVATE_KEY, private_key, strlen(private_key));

	if (err) {
		LOG_ERR("Failed to install private key, error: %d", err);
		return err;
	}
#endif

	return 0;
}
#endif /* CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES */

#if defined(CONFIG_NRF_CLOUD_CHECK_CREDENTIALS)
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

	/* A client certificate is required for MQTT for the mutual-TLS connection */
	if (IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)) {
		if (!cs.client_cert) {
			LOG_ERR("Client Certificate not found in sec tag %d", cs.sec_tag);
			ret = -ENOTSUP;
		}
	}

	return ret;
}

static int cred_exists(uint32_t sec_tag, int type, bool *exists)
{
	if (exists == NULL) {
		return -EINVAL;
	}

	int err;

#if defined(CONFIG_NRF_CLOUD_CREDENTIALS_MGMT_MODEM)
	err = modem_key_mgmt_exists(sec_tag, (enum modem_key_mgmt_cred_type)cred_type[type],
				    exists);
	if (err) {
		LOG_ERR("modem_key_mgmt_exists() failed for type %d in sec tag %u, error: %d",
			(enum modem_key_mgmt_cred_type)cred_type[type], sec_tag, err);
	}
#else
	size_t credlen = 0;

	err = tls_credential_get(sec_tag, (enum tls_credential_type)cred_type[type],
				 NULL, &credlen);
	*exists = (err == -ENOENT) ? false : true;
	err = 0;
#endif

	return err;
}

int nrf_cloud_credentials_check(struct nrf_cloud_credentials_status *const cs)
{
	if (!cs) {
		return -EINVAL;
	}

	int ret;
	bool exists = false;

	memset(cs, 0, sizeof(*cs));

	cs->sec_tag = CONFIG_NRF_CLOUD_SEC_TAG;

#if defined(CONFIG_NRF_CLOUD_COAP)
	cs->sec_tag = CONFIG_NRF_CLOUD_COAP_SEC_TAG;
#endif

	ret = cred_exists(cs->sec_tag, CA_CERT, &exists);
	if (ret < 0) {
		return -EIO;
	}
	cs->ca = exists;

	ret = cred_exists(cs->sec_tag, CLIENT_CERT, &exists);
	if (ret < 0) {
		return -EIO;
	}
	cs->client_cert = exists;

	ret = cred_exists(cs->sec_tag, PRIVATE_KEY, &exists);
	if (ret < 0) {
		return -EIO;
	}
	cs->prv_key = exists;

	LOG_DBG("Sec Tag: %u, CA: %s, Client Cert: %s, Private Key: %s",
		cs->sec_tag,
		cs->ca		? "Yes" : "No",
		cs->client_cert	? "Yes" : "No",
		cs->prv_key	? "Yes" : "No");

	return 0;
}
#endif /* CONFIG_NRF_CLOUD_CHECK_CREDENTIALS */
