/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_MODEM_JWT)
#include <modem/modem_jwt.h>
#include <nrf_modem_at.h>
#endif
#include "nrf_cloud_client_id.h"

#define GET_TIME_CMD		"AT%%CCLK?"
/* Example CCLK response */
#define GET_TIME_RSP_STR	"%CCLK: \"24/07/03,21:29:34-28\",1\r\nOK\r\n"
/* Expected size of the response, plus some extra padding */
#define GET_TIME_RSP_SZ		(sizeof(GET_TIME_RSP_STR) + 5)

LOG_MODULE_REGISTER(nrf_cloud_jwt, CONFIG_NRF_CLOUD_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_JWT_SOURCE_CUSTOM)
#include <zephyr/sys/base64.h>
#include <psa/crypto.h>
#include <app_jwt.h>

/* Size of the ES256 private key material */
#define PRV_KEY_SZ		(32)

#define PRV_KEY_DER_SZ		(138)
#define PRV_KEY_PEM_SZ		(256)
#define PRV_KEY_DER_START_IDX	(36)

static void remove_newlines(char *const str)
{
	size_t new;
	size_t len = strlen(str);

	for (size_t old = new = 0; old < len; ++old) {
		if (str[old] != '\n') {
			str[new++] = str[old];
		}
	}

	str[new] = '\0';
}

#define BEGIN_PRV_KEY "-----BEGIN PRIVATE KEY-----"
#define END_PRV_KEY "-----END PRIVATE KEY-----"
static int strip_non_key_data(char *const str)
{
	char *start;
	char *end;
	size_t new_len;

	/* Find and remove end string */
	end = strstr(str, END_PRV_KEY);
	if (end == NULL) {
		return -EINVAL;
	}
	*end = '\0';

	/* Find and remove begin string */
	start = strstr(str, BEGIN_PRV_KEY);
	if (start == NULL) {
		return -EINVAL;
	}

	start += strlen(BEGIN_PRV_KEY);
	new_len = strlen(start);

	/* Move key data to the front */
	memmove(str, start, new_len);
	str[new_len] = '\0';

	remove_newlines(str);

	return 0;
}

static int get_key_from_cred(const int sec_tag, uint8_t *const der_out)
{
	static char pem[PRV_KEY_PEM_SZ];
	static uint8_t der[PRV_KEY_DER_SZ];
	size_t pem_sz = sizeof(pem);
	size_t der_sz;
	int err;

	/* Get the private key from the TLS credentials subsystem.
	 * This function only supports keys in PEM format.
	 */
	err = tls_credential_get(sec_tag, TLS_CREDENTIAL_PRIVATE_KEY,
				 pem, &pem_sz);
	if (err) {
		LOG_ERR("tls_credential_get() failed, error: %d", err);
		return err;
	}

	err = strip_non_key_data(pem);
	if (err) {
		LOG_ERR("Failed to parse PEM file, error: %d", err);
		return err;
	}

	LOG_DBG("PEM:\n%s", pem);
	pem_sz = strlen(pem);

	/* Convert the PEM to DER (binary) */
	err = base64_decode(der, sizeof(der), &der_sz, pem, pem_sz);
	if (err) {
		LOG_ERR("base64_decode() failed, error: %d", err);
		return -EBADF;
	}

	LOG_DBG("DER size = %u", der_sz);

	const uint8_t expected_der_header[] = {0x30, 0x81, 0x87, 0x02};

	/* NOTE: Hack, not actually parsing the ASN.1,
	 * just verifying that key header has the expected format
	 */
	if (memcmp(der, expected_der_header, sizeof(expected_der_header))) {
		LOG_ERR("Unexpected DER format: 0x%X 0x%X 0x%X", der[0], der[1], der[2]);
		return -EBADF;
	}

	/* Copy the private key material */
	memcpy((void *)der_out, (void *)&der[PRV_KEY_DER_START_IDX], PRV_KEY_SZ);

	return 0;
}

static int custom_jwt_generate(uint32_t exp_delta_s, char *const jwt_buf, size_t jwt_buf_sz,
			       const char *subject, int sec_tag)
{
	int err = 0;
	psa_key_id_t kid;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t priv_key[PRV_KEY_SZ];

	/* Load private key from storage */
	err = get_key_from_cred(sec_tag, priv_key);
	if (err) {
		LOG_ERR("Failed to get private key, error: %d", err);
		return err;
	}

	err = psa_crypto_init();
	if (err != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed! (Error: %d)", err);
		return err;
	}

	psa_set_key_usage_flags(&key_attributes,
				PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	/* Import key into PSA */
	err = psa_import_key(&key_attributes, priv_key, sizeof(priv_key), &kid);
	if (err != PSA_SUCCESS) {
		LOG_ERR("psa_import_key failed! (Error: %d)", err);
		return err;
	}

	struct app_jwt_data _jwt_internal = {
		.sec_tag = kid,
		.key_type = JWT_KEY_TYPE_CLIENT_PRIV,
		.alg = JWT_ALG_TYPE_ES256,
		.validity_s = exp_delta_s,
		.jwt_buf = jwt_buf,
		.jwt_sz = jwt_buf_sz,
		.subject = subject,
	};

	return app_jwt_generate(&_jwt_internal);
}
#endif /* CONFIG_NRF_CLOUD_JWT_SOURCE_CUSTOM */

int nrf_cloud_jwt_generate(uint32_t time_valid_s, char *const jwt_buf, size_t jwt_buf_sz)
{
	if (!jwt_buf || !jwt_buf_sz) {
		return -EINVAL;
	}

	int err;
	const char *id_ptr;
	uint32_t exp_delta_s = time_valid_s;
	int sec_tag = IS_ENABLED(CONFIG_NRF_CLOUD_COAP) ?
		      nrf_cloud_sec_tag_coap_jwt_get() : nrf_cloud_sec_tag_get();
	const char *subject;

#if defined(CONFIG_MODEM_JWT)
	/* Check if modem time is valid */
	char buf[GET_TIME_RSP_SZ];

	err = nrf_modem_at_cmd(buf, sizeof(buf), GET_TIME_CMD);
	if (err != 0) {
		LOG_ERR("Modem does not have valid date/time, JWT not generated");
		return -ETIME;
	}
#endif
	if (time_valid_s > NRF_CLOUD_JWT_VALID_TIME_S_MAX) {
		exp_delta_s = NRF_CLOUD_JWT_VALID_TIME_S_MAX;
	} else if (time_valid_s == 0) {
		exp_delta_s = NRF_CLOUD_JWT_VALID_TIME_S_DEF;
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID)) {
		/* The UUID is present in the iss claim, so there is no need
		 * to also include it in the sub claim.
		 */
		subject = NULL;
	} else {
		err = nrf_cloud_client_id_ptr_get(&id_ptr);
		if (err) {
			LOG_ERR("Failed to obtain client ID, error: %d", err);
			return err;
		}
		subject = id_ptr;
	}

#if defined(CONFIG_NRF_CLOUD_JWT_SOURCE_CUSTOM)
	return custom_jwt_generate(exp_delta_s, jwt_buf, jwt_buf_sz, subject, sec_tag);
#elif defined(CONFIG_MODEM_JWT)
	struct jwt_data jwt = {
		.audience = NULL,
		.key = JWT_KEY_TYPE_CLIENT_PRIV,
		.alg = JWT_ALG_TYPE_ES256,
		.jwt_buf = jwt_buf,
		.jwt_sz = jwt_buf_sz,
		.exp_delta_s = exp_delta_s,
		.sec_tag = sec_tag,
		.subject = subject,
	};
	err = modem_jwt_generate(&jwt);
	if (err) {
		LOG_ERR("Failed to generate JWT, error: %d", err);
	}
#else
	LOG_ERR("Invalid configuration for JWT generation");
	err = -ENOTSUP;
#endif

	return err;
}
