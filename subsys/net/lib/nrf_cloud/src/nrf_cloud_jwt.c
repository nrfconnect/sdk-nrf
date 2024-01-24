/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/modem_jwt.h>
#if defined(CONFIG_MODEM_JWT)
#include <nrf_modem_at.h>
#endif

#define GET_TIME_CMD "AT%%CCLK?"

LOG_MODULE_REGISTER(nrf_cloud_jwt, CONFIG_NRF_CLOUD_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_JWT_SOURCE_CUSTOM)
#include <zephyr/sys/base64.h>
#include <zephyr/random/random.h>

#include <tinycrypt/ctr_prng.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/ecc_dsa.h>
#include <tinycrypt/constants.h>

#include <date_time.h>
#include <cJSON.h>

#include "nrf_cloud_mem.h"

#define ECDSA_SHA_256_SIG_SZ	(64)
#define ECDSA_SHA_256_HASH_SZ	(32)
#define BASE64_ENCODE_SZ(n)	(((4 * n / 3) + 3) & ~3)
#define B64_SIG_SZ		(BASE64_ENCODE_SZ(ECDSA_SHA_256_SIG_SZ) + 1)

/* JWT header for ES256 type: {"typ":"JWT","alg":"ES256"} */
#define JWT_ES256_HDR		"eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiJ9"

/* Size of the ES256 private key material */
#define PRV_KEY_SZ		(32)

#define PRV_KEY_DER_SZ		(138)
#define PRV_KEY_PEM_SZ		(256)
#define PRV_KEY_DER_START_IDX	(36)

static TCCtrPrng_t prng_state;

static int setup_prng(void)
{
	static bool prng_init;
	uint8_t entropy[TC_AES_KEY_SIZE + TC_AES_BLOCK_SIZE];

	if (prng_init) {
		return 0;
	}

	for (int i = 0; i < sizeof(entropy); i += sizeof(uint32_t)) {
		uint32_t rv = sys_rand32_get();

		memcpy(entropy + i, &rv, sizeof(uint32_t));
	}

	int res = tc_ctr_prng_init(&prng_state,
				   (const uint8_t *)&entropy, sizeof(entropy),
				   __FILE__, sizeof(__FILE__));

	if (res != TC_CRYPTO_SUCCESS) {
		LOG_ERR("tc_ctr_prng_init() failed, error: %d", res);
		return -EINVAL;
	}

	prng_init = true;

	return 0;
}

int default_CSPRNG(uint8_t *dest, unsigned int size)
{
	return tc_ctr_prng_generate(&prng_state, NULL, 0, dest, size);
}

static void base64_url_format(char *const base64_string)
{
	if (base64_string == NULL) {
		return;
	}

	char *found = NULL;

	/* replace '+' with "-" */
	for (found = base64_string; (found = strchr(found, '+'));) {
		*found = '-';
	}

	/* replace '/' with "_" */
	for (found = base64_string; (found = strchr(found, '/'));) {
		*found = '_';
	}

	/* remove padding '=' */
	found = strchr(base64_string, '=');
	if (found) {
		*found = '\0';
	}
}

static int sign_message_der(const char *const der_key, const char *const msg_in,
			    char *const sig_out)
{
	struct tc_sha256_state_struct ctx;
	uint8_t hash[ECDSA_SHA_256_HASH_SZ];
	int res;

	tc_sha256_init(&ctx);
	tc_sha256_update(&ctx, msg_in, strlen(msg_in));
	tc_sha256_final(hash, &ctx);

	res = setup_prng();
	if (res != 0) {
		LOG_ERR("uECC_sign() failed, error: %d", res);
		return res;
	}

	LOG_HEXDUMP_DBG(hash, sizeof(hash), "SHA256 hash: ");

	uECC_set_rng(&default_CSPRNG);

	/* Note that tinycrypt only supports P-256. */
	res = uECC_sign(der_key, hash, sizeof(hash), sig_out, &curve_secp256r1);
	if (res != TC_CRYPTO_SUCCESS) {
		LOG_ERR("uECC_sign() failed, error: %d", res);
		return -EINVAL;
	}

	return 0;
}

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

static int get_key_from_cred(const int sec_tag, char *const der_out)
{
	static char pem[PRV_KEY_PEM_SZ];
	static char der[PRV_KEY_DER_SZ];
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

	/* NOTE: Hack, not actually parsing the ASN.1,
	 * just verifying that key header has the expected format
	 */
	if (((der[0] == 0x30) && (der[1] == 0x81) && (der[2] == 0x87)) == false) {
		LOG_ERR("Unexpected DER format: 0x%X 0x%X 0x%X", der[0], der[1], der[2]);
		return -EBADF;
	}

	/* Copy the private key material */
	memcpy((void *)der_out, (void *)&der[PRV_KEY_DER_START_IDX], PRV_KEY_SZ);

	return 0;
}

static char *jwt_payload_create(const char *const sub, const char *const aud, int64_t exp)
{
	int err = 0;
	char *pay_str = NULL;
	cJSON *jwt_pay = cJSON_CreateObject();

	if (sub && (cJSON_AddStringToObjectCS(jwt_pay, "sub", sub) == NULL)) {
		err = -ENOMEM;
	}

	if (aud && (cJSON_AddStringToObjectCS(jwt_pay, "aud", aud) == NULL)) {
		err = -ENOMEM;
	}

	if (exp > 0) {
		/* Add expiration (exp) claim */
		if (cJSON_AddNumberToObjectCS(jwt_pay, "exp", exp) == NULL) {
			err = -ENOMEM;
		}
	}

	if (err == 0) {
		pay_str = cJSON_PrintUnformatted(jwt_pay);
	}

	cJSON_Delete(jwt_pay);
	jwt_pay = NULL;

	return pay_str;
}

static char *convert_str_to_b64_url(const char *const str)
{
	size_t str_len = strlen(str);
	size_t b64_sz = BASE64_ENCODE_SZ(str_len) + 1;

	char *b64_out = nrf_cloud_calloc(1, b64_sz);

	if (b64_out) {
		int err = base64_encode(b64_out, b64_sz, &b64_sz, str, strlen(str));

		if (err) {
			LOG_ERR("base64_encode failed, error: %d", err);
		}
	}

	/* Convert to base64 URL */
	base64_url_format(b64_out);

	return b64_out;
}

static char *jwt_header_payload_combine(const char *const hdr, const char *const pay)
{
	/* Allocate buffer for the JWT header and payload to be signed */
	size_t msg_sz = strlen(hdr) + 1 + strlen(pay) + 1;
	char *msg_out = nrf_cloud_calloc(1, msg_sz);

	/* Build the base64 URL JWT to sign:
	 * <base64_header>.<base64_payload>
	 */
	if (msg_out) {
		int ret = snprintk(msg_out, msg_sz, "%s.%s", hdr, pay);

		if ((ret < 0) || (ret >= msg_sz)) {
			LOG_ERR("Could not format JWT to be signed");
			nrf_cloud_free(msg_out);
			msg_out = NULL;
		}
	}

	return msg_out;
}

static char *unsigned_jwt_create(struct jwt_data *const jwt)
{
	int err = 0;
	int64_t exp_ts_s;

	char *pay_str;
	char *pay_b64;
	char *unsigned_jwt;

	/* Get expiration time stamp */
	if (jwt->exp_delta_s > 0) {
		err = date_time_now(&exp_ts_s);
		if (!err) {
			exp_ts_s += jwt->exp_delta_s;

		} else {
			LOG_ERR("date_time_now() failed, error: %d", err);
			return NULL;
		}
	}

	/* Create the payload */
	pay_str = jwt_payload_create(jwt->subject, jwt->audience, exp_ts_s);
	if (!pay_str) {
		LOG_ERR("Failed to create JWT JSON payload");
		return NULL;
	}

	/* Convert payload JSON string to base64 URL */
	pay_b64 = convert_str_to_b64_url(pay_str);

	cJSON_free(pay_str);
	pay_str = NULL;

	if (!pay_b64) {
		LOG_ERR("Failed to convert string to base64");
		return NULL;
	}

	/* Create the base64 URL data to be signed */
	unsigned_jwt = jwt_header_payload_combine(JWT_ES256_HDR, pay_b64);

	nrf_cloud_free(pay_b64);
	pay_b64 = NULL;

	return unsigned_jwt;
}

static int jwt_signature_get(const int sec_tag, const char *const jwt,
			     char *const sig_buf, size_t sig_sz)
{
	int err;
	size_t o_len;
	char der_buf[PRV_KEY_SZ];
	uint8_t sig_raw[ECDSA_SHA_256_SIG_SZ];

	/* Get the key which will be used to sign the JWT */
	err = get_key_from_cred(sec_tag, der_buf);
	if (err) {
		LOG_ERR("Failed to get private key material, error: %d", err);
		return -EIDRM;
	}

	err = sign_message_der(der_buf, jwt, sig_raw);
	if (err) {
		LOG_ERR("Failed to sign JWT, error: %d", err);
		return -ENOEXEC;
	}

	LOG_HEXDUMP_DBG(sig_raw, sizeof(sig_raw), "Signature: ");

	/* Convert signature to base64 URL */
	err = base64_encode(sig_buf, sig_sz, &o_len, sig_raw, sizeof(sig_raw));
	if (err) {
		LOG_ERR("base64_encode failed, error: %d", err);
		return -EIO;
	}

	base64_url_format(sig_buf);
	return 0;
}

static int jwt_signature_append(const char *const unsigned_jwt, const char *const sig,
				char *const jwt_buf, size_t jwt_sz)
{
	int err = 0;

	/* Get the size of the final, signed JWT: +1 for '.' and null-terminator */
	size_t final_sz = strlen(unsigned_jwt) + 1 + strlen(sig) + 1;

	if (final_sz > jwt_sz) {
		/* Provided buffer is too small */
		err = -E2BIG;
	}

	/* JWT final form:
	 * <base64_header>.<base64_payload>.<base64_signature>
	 */
	if (err == 0) {
		int ret = snprintk(jwt_buf, jwt_sz, "%s.%s", unsigned_jwt, sig);

		if ((ret < 0) || (ret >= jwt_sz)) {
			err = -ETXTBSY;
		}
	}

	return err;
}

static int custom_jwt_generate(struct jwt_data *const jwt)
{
	int err = 0;
	char *unsigned_jwt;
	uint8_t jwt_sig[B64_SIG_SZ];

	/* Create the JWT to be signed */
	unsigned_jwt = unsigned_jwt_create(jwt);
	if (!unsigned_jwt) {
		LOG_ERR("Failed to create JWT to be signed");
		return -EIO;
	}

	LOG_DBG("Message to sign: %s", unsigned_jwt);

	/* Get the signature of the unsigned JWT */
	err = jwt_signature_get(jwt->sec_tag, unsigned_jwt, jwt_sig, sizeof(jwt_sig));
	if (err) {
		LOG_ERR("Failed to get JWT signature, error: %d", err);
		nrf_cloud_free(unsigned_jwt);
		return -ENOEXEC;
	}

	/* Append the signature, creating the complete JWT */
	err = jwt_signature_append(unsigned_jwt, jwt_sig, jwt->jwt_buf, jwt->jwt_sz);

	nrf_cloud_free(unsigned_jwt);

	if (!err) {
		LOG_DBG("JWT:\n%s", jwt->jwt_buf);
	} else {
		LOG_ERR("Failed to append JWT signature, error: %d", err);
	}

	return err;
}
#endif /* CONFIG_NRF_CLOUD_JWT_SOURCE_CUSTOM */

int nrf_cloud_jwt_generate(uint32_t time_valid_s, char *const jwt_buf, size_t jwt_buf_sz)
{
	if (!jwt_buf || !jwt_buf_sz) {
		return -EINVAL;
	}

	int err;
	char buf[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];
	struct jwt_data jwt = {
		.audience = NULL,
#if defined(CONFIG_NRF_CLOUD_COAP)
		.sec_tag = CONFIG_NRF_CLOUD_COAP_SEC_TAG,
#else
		.sec_tag = CONFIG_NRF_CLOUD_SEC_TAG,
#endif
		.key = JWT_KEY_TYPE_CLIENT_PRIV,
		.alg = JWT_ALG_TYPE_ES256,
		.jwt_buf = jwt_buf,
		.jwt_sz = jwt_buf_sz
	};

#if defined(CONFIG_MODEM_JWT)
	/* Check if modem time is valid */
	err = nrf_modem_at_cmd(buf, sizeof(buf), GET_TIME_CMD);
	if (err != 0) {
		LOG_ERR("Modem does not have valid date/time, JWT not generated");
		return -ETIME;
	}
#endif
	if (time_valid_s > NRF_CLOUD_JWT_VALID_TIME_S_MAX) {
		jwt.exp_delta_s = NRF_CLOUD_JWT_VALID_TIME_S_MAX;
	} else if (time_valid_s == 0) {
		jwt.exp_delta_s = NRF_CLOUD_JWT_VALID_TIME_S_DEF;
	} else {
		jwt.exp_delta_s = time_valid_s;
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID)) {
		/* The UUID is present in the iss claim, so there is no need
		 * to also include it in the sub claim.
		 */
		jwt.subject = NULL;
	} else {
		err = nrf_cloud_client_id_get(buf, sizeof(buf));
		if (err) {
			LOG_ERR("Failed to obtain client id, error: %d", err);
			return err;
		}
		jwt.subject = buf;
	}

#if defined(CONFIG_NRF_CLOUD_JWT_SOURCE_CUSTOM)
	return custom_jwt_generate(&jwt);
#elif defined(CONFIG_MODEM_JWT)
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
