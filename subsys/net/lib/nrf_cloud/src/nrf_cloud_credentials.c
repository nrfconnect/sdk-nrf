/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <net/nrf_cloud.h>
#include <zephyr/logging/log.h>
#include <modem/modem_key_mgmt.h>

LOG_MODULE_REGISTER(nrf_cloud_credentials, CONFIG_NRF_CLOUD_LOG_LEVEL);

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
	bool exists = false;

	memset(cs, 0, sizeof(*cs));

	cs->sec_tag = CONFIG_NRF_CLOUD_SEC_TAG;

#if defined(CONFIG_NRF_CLOUD_COAP_SEC_TAG)
	cs->sec_tag = CONFIG_NRF_CLOUD_COAP_SEC_TAG;
#endif

	ret = modem_key_mgmt_exists(cs->sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (ret < 0) {
		LOG_ERR("modem_key_mgmt_exists() failed for CA cert in sec tag %u, error: %d",
			cs->sec_tag, ret);
		return -EIO;
	}
	cs->ca = exists;

	ret = modem_key_mgmt_exists(cs->sec_tag, MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT, &exists);
	if (ret < 0) {
		LOG_ERR("modem_key_mgmt_exists() failed for client cert in sec tag %u, error: %d",
			cs->sec_tag, ret);
		return -EIO;
	}
	cs->client_cert = exists;

	ret = modem_key_mgmt_exists(cs->sec_tag, MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT, &exists);
	if (ret < 0) {
		LOG_ERR("modem_key_mgmt_exists() failed for private key in sec tag %u, error: %d",
			cs->sec_tag, ret);
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
