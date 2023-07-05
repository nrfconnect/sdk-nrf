/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/net/socket.h>
#include <modem/modem_key_mgmt.h>
#include "slm_native_tls.h"

LOG_MODULE_REGISTER(slm_tls, CONFIG_SLM_LOG_LEVEL);

/* max buffer length to load credential */
#define MAX_CRDL_LEN		4096

/* pointer to credential buffer */
static uint8_t *crdl;

/**
 * @brief Map SLM security tag to nRF security tag
 */
nrf_sec_tag_t slm_tls_map_sectag(sec_tag_t sec_tag, uint16_t type)
{
	if (sec_tag > MAX_SLM_SEC_TAG || sec_tag < MIN_SLM_SEC_TAG) {
		LOG_ERR("Invalid security tag.");
		return -EINVAL;
	}

	return sec_tag*10 + type;
}

/**
 * @brief Load TLS credentials
 */
int slm_tls_loadcrdl(sec_tag_t sec_tag)
{
	int ret, len = MAX_CRDL_LEN, offset = 0;
	bool loaded = false;

	if (sec_tag > MAX_SLM_SEC_TAG || sec_tag < MIN_SLM_SEC_TAG) {
		LOG_ERR("Invalid security tag.");
		return -EINVAL;
	}

	crdl = k_malloc(MAX_CRDL_LEN);
	if (crdl == NULL) {
		LOG_ERR("Fail to allocate memory");
		return -ENOMEM;
	}
	memset(crdl, 0, MAX_CRDL_LEN);

	/* Load CA certificate */
	ret = modem_key_mgmt_read(slm_tls_map_sectag(sec_tag, 0),
				  0, crdl + offset, &len);
	if (ret == 0) {
		LOG_DBG("Load CA cert %d: Len: %d",
			slm_tls_map_sectag(sec_tag, 0), len);
		len++;
		ret = tls_credential_add(sec_tag, TLS_CREDENTIAL_CA_CERTIFICATE,
					crdl + offset, len);
		if (ret != 0) {
			LOG_ERR("Failed to register CA certificate: %d", ret);
			goto exit;
		}
		offset += len;
		len = MAX_CRDL_LEN - offset;
		loaded = true;
	} else {
		LOG_DBG("Empty CA cert at %d:", slm_tls_map_sectag(sec_tag, 0));
	}

	/* Load server/client certificate */
	ret = modem_key_mgmt_read(slm_tls_map_sectag(sec_tag, 1), 0,
				  crdl + offset, &len);
	if (ret == 0) {
		LOG_DBG("Load cert %d. Len: %d",
			slm_tls_map_sectag(sec_tag, 1), len);
		len++;
		ret = tls_credential_add(sec_tag,
					TLS_CREDENTIAL_SERVER_CERTIFICATE,
					crdl + offset, len);
		if (ret < 0) {
			LOG_ERR("Failed to register public cert: %d", ret);
			goto exit;
		}
		offset += len;
		len = MAX_CRDL_LEN - offset;
		loaded = true;
	} else {
		LOG_DBG("Empty cert at %d:", slm_tls_map_sectag(sec_tag, 1));
	}

	/* Load private key */
	ret = modem_key_mgmt_read(slm_tls_map_sectag(sec_tag, 2), 0,
				  crdl + offset, &len);
	if (ret == 0) {
		LOG_DBG("Load private key %d. Len: %d",
			slm_tls_map_sectag(sec_tag, 2), len);
		len++;
		ret = tls_credential_add(sec_tag, TLS_CREDENTIAL_PRIVATE_KEY,
					crdl + offset, len);
		if (ret < 0) {
			LOG_ERR("Failed to register private key: %d", ret);
			goto exit;
		}
		loaded = true;
	} else {
		LOG_DBG("Empty private key at %d:",
			slm_tls_map_sectag(sec_tag, 2));
	}

	if (loaded) {
		/* Load credential successfully */
		return 0;
	}
	LOG_ERR("No credential for sec_tag:%d", sec_tag);
	ret = -EINVAL;

exit:
	k_free(crdl);
	crdl = NULL;

	return ret;
}

/**
 * @brief Unload TLS credentials
 */
int slm_tls_unloadcrdl(sec_tag_t sec_tag)
{
	if (sec_tag > MAX_SLM_SEC_TAG || sec_tag < MIN_SLM_SEC_TAG) {
		LOG_ERR("Invalid security tag.");
		return -EINVAL;
	}
	tls_credential_delete(sec_tag, TLS_CREDENTIAL_CA_CERTIFICATE);
	tls_credential_delete(sec_tag, TLS_CREDENTIAL_SERVER_CERTIFICATE);
	tls_credential_delete(sec_tag, TLS_CREDENTIAL_PRIVATE_KEY);

	k_free(crdl);
	crdl = NULL;

	return 0;
}
