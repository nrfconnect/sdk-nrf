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
	const uint32_t sec_tag = nrf_cloud_sec_tag_get();

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

	if (IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) || IS_ENABLED(CONFIG_NRF_CLOUD_REST)) {
		if (!cs.ca_aws && cs.ca_coap) {
			/* There is a CA, but not large enough to be correct */
			LOG_WRN("Connection using MQTT or REST may fail as the size of the CA "
				"cert indicates it is a CoAP root CA.");
			ret = -ENOPROTOOPT;
		}
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_COAP)) {
		if (!cs.ca_coap && cs.ca_aws) {
			LOG_WRN("Connection using CoAP "
				"may fail as the size of the CA cert indicates "
				"CoAP CA certificate is missing but AWS CA is present.");
			ret = -ENOPROTOOPT;
		} else if (!IS_ENABLED(CONFIG_NRF_CLOUD_COAP_DOWNLOADS)) {
			if (cs.ca && (!cs.ca_coap || !cs.ca_aws)) {
				LOG_WRN("Connection using CoAP and downloading using HTTP "
					"may fail as the size of the CA cert indicates both "
					"CoAP and AWS root CA certs are not present.");
				ret = -ENOPROTOOPT;
			}
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

static int cred_size_get(uint32_t sec_tag, int type, size_t *cred_sz)
{
	int err;

	*cred_sz = 0; /* We just want to determine the size */

#if defined(CONFIG_NRF_CLOUD_CREDENTIALS_MGMT_MODEM)
	uint8_t buf[1];

	err = modem_key_mgmt_read(sec_tag, (enum modem_key_mgmt_cred_type)cred_type[type],
				  buf, cred_sz);
	if (err && (err != -ENOMEM)) {
		LOG_ERR("modem_key_mgmt_read() failed for type %d in sec tag %u, error: %d",
			(enum modem_key_mgmt_cred_type)cred_type[type], sec_tag, err);
	} else {
		err = 0;
	}
#else
	err = tls_credential_get(sec_tag, (enum tls_credential_type)cred_type[type],
				 NULL, cred_sz);
	if (err == -EFBIG) { /* Error expected since we only want the size */
		err = 0;
	}
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

	cs->sec_tag = nrf_cloud_sec_tag_get();

	ret = cred_exists(cs->sec_tag, CA_CERT, &exists);
	if (ret < 0) {
		LOG_ERR("Error checking CA exists");
		return -EIO;
	}
	cs->ca = exists;
	if (exists) {
		ret = cred_size_get(cs->sec_tag, CA_CERT, &cs->ca_size);
		if (ret < 0) {
			LOG_ERR("Error checking CA size");
			return -EIO;
		}
		/* These flags are approximate and only useful for logging to help diagnose
		 * possible provisioning mistakes.
		 */
		size_t coap_min_sz = CONFIG_NRF_CLOUD_COAP_CA_CERT_SIZE_THRESHOLD;
		size_t aws_min_sz = CONFIG_NRF_CLOUD_AWS_CA_CERT_SIZE_THRESHOLD;
		size_t combined_min_sz = aws_min_sz + coap_min_sz;

		if (cs->ca_size > combined_min_sz) {
			cs->ca_aws = true;
			cs->ca_coap = true;
		} else if (cs->ca_size > aws_min_sz) {
			cs->ca_aws = true;
		} else if (cs->ca_size > coap_min_sz) {
			cs->ca_coap = true;
		}
	}

	ret = cred_exists(cs->sec_tag, CLIENT_CERT, &exists);
	if (ret < 0) {
		LOG_ERR("Error checking client cert exists");
		return -EIO;
	}
	cs->client_cert = exists;

	ret = cred_exists(cs->sec_tag, PRIVATE_KEY, &exists);
	if (ret < 0) {
		LOG_ERR("Error checking private key exists");
		return -EIO;
	}
	cs->prv_key = exists;

	LOG_INF("Sec Tag: %u; CA: %s, Client Cert: %s, Private Key: %s",
		cs->sec_tag,
		cs->ca		? "Yes" : "No",
		cs->client_cert	? "Yes" : "No",
		cs->prv_key	? "Yes" : "No");
	LOG_INF("CA Size: %zd, AWS: %s, CoAP: %s",
		cs->ca_size,
		cs->ca_aws ? "Likely" : "Unlikely",
		cs->ca_coap ? "Likely" : "Unlikely");

	return 0;
}
#endif /* CONFIG_NRF_CLOUD_CHECK_CREDENTIALS */
