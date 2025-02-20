/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sdfw/sdfw_services/device_info_service.h>

#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <psa/nrf_platform_key_ids.h>

#include <cJSON.h>

#include <app_jwt.h>

#include <zephyr/posix/time.h>
#include <zephyr/sys/base64.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_jwt, CONFIG_APP_JWT_LOG_LEVEL);

/* Size of a UUID in words */
#define UUID_BINARY_WORD_SZ (4)

/* Size of a UUID in bytes */
#define UUID_BINARY_BYTES_SZ (16)

/* String size of a binary word encoded in hexadecimal */
#define BINARY_WORD_STR_SZ (9)

/* Size of an ECDSA signature */
#define ECDSA_SHA_256_SIG_SZ (64)

/* Size of an SHA 256 hash */
#define ECDSA_SHA_256_HASH_SZ (32)

/* Size of a public ECDSA key in raw binary format  */
#define ECDSA_PUBLIC_KEY_SZ (65)

/* Size of a public ECDSA key in DER format  */
#define ECDSA_PUBLIC_KEY_DER_SZ (91)

/* Macro to determine the size of a data encoded in base64 */
#define BASE64_ENCODE_SZ(n) (((4 * n / 3) + 3) & ~3)

/* Size of ECDSA signature encoded in base64 */
#define B64_SIG_SZ (BASE64_ENCODE_SZ(ECDSA_SHA_256_SIG_SZ) + 1)

/* Length of random bits added to JTI field */
#define JWT_NONCE_BITS (128)

/* Length of JTI field, this should be at least UUIDv4 size
 * + size of board name
 */
#define JWT_CLAIM_FILED_STR_LENGTH (64)

static void base64_url_format(char *const base64_string)
{
	if (base64_string == NULL) {
		return;
	}

	char *found = NULL;

	/* Replace '+' with "-" */
	for (found = base64_string; (found = strchr(found, '+'));) {
		*found = '-';
	}

	/* Replace '/' with "_" */
	for (found = base64_string; (found = strchr(found, '/'));) {
		*found = '_';
	}

	/* Remove padding '=' */
	found = strchr(base64_string, '=');
	if (found) {
		*found = '\0';
	}
}

static int bytes_to_uuid_str(const uint32_t *uuid_words, const int32_t uuid_byte_len,
			     char *uuid_str_out, const int32_t uuid_str_out_size)
{

	if ((NULL == uuid_words) || (uuid_byte_len < UUID_BINARY_WORD_SZ * 4) ||
	    (NULL == uuid_str_out) || (uuid_str_out_size < APP_JWT_UUID_V4_STR_LEN)) {
		/* Bad parameter */
		return -EINVAL;
	}
	/* This will hold 4 integer words in string format */
	/*(string Length 8 + 1) */
	char temp_str[UUID_BINARY_WORD_SZ][BINARY_WORD_STR_SZ] = {0};

	/* Transform integers to strings */
	for (int i = 0; i < UUID_BINARY_WORD_SZ; i++) {
		/* UUID byte endiannes is little endian first. */
		sprintf(temp_str[i], "%08x", sys_cpu_to_be32(uuid_words[i]));
	}

	/* UUID string format defined by  ITU-T X.667 | ISO/IEC 9834-8 : */
	/* <8 char>-<4 char>-<4 char>-<4 char>-<12 char> */

	sprintf(uuid_str_out, "%.8s-%.4s-%.4s-%.4s-%.4s%.8s",
		temp_str[0], temp_str[1] + 0, temp_str[1] + 4,
		temp_str[2] + 0, temp_str[2] + 4, temp_str[3]);

	uuid_str_out[APP_JWT_UUID_V4_STR_LEN] = '\0';

	return 0;
}

static int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed! (Error: %d)", status);
		return -ENOMEM;
	}

	return 0;
}

static int raw_ecc_pubkey_to_der(uint8_t *raw_pub_key, uint8_t *der_pub_key,
				 const size_t der_pub_key_buffer_size, size_t *der_pub_key_len)
{
	int err = -1;

	if (ECDSA_PUBLIC_KEY_DER_SZ <= der_pub_key_buffer_size) {
		uint8_t der_pubkey_header[27] = {
			/* Integer sequence of 89 bytes */
			0x30, 0x59,
			/* Integer sequence of 19 bytes */
			0x30, 0x13,
			/* ecPublicKey (ANSI X9.62 public key type) */
			0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01,
			/* prime256v1 (ANSI X9.62 named elliptic curve) */
			0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07,
			/* Integer sequence of 66 bytes */
			0x03, 0x42,
			/* Header of uncompressed RAW public ECC key */
			0x00, 0x04};

		memcpy(der_pub_key, der_pubkey_header, sizeof(der_pubkey_header));
		memcpy((der_pub_key + 27), (raw_pub_key + 1), ECDSA_PUBLIC_KEY_SZ - 1);

		*der_pub_key_len = ECDSA_PUBLIC_KEY_DER_SZ;
		err = 0;
	}

	return err;
}

static int export_public_key_hash(const uint32_t user_key_id, uint8_t *key_hash_out,
				  size_t key_hash_buffer_size, size_t *key_hash_Length)
{
	int err;

	psa_status_t status;

	size_t olen;

	uint8_t pub_key[ECDSA_PUBLIC_KEY_SZ];

	/* Export the public key */
	status = psa_export_public_key(user_key_id, pub_key, sizeof(pub_key), &olen);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_export_public_key failed! (Error: %d)", status);
		return -1;
	}

	uint8_t pubkey_der[ECDSA_PUBLIC_KEY_DER_SZ] = {0};
	size_t pubkey_der_len = 0;

	err = raw_ecc_pubkey_to_der(pub_key, pubkey_der, sizeof(pubkey_der), &pubkey_der_len);
	if (err != 0) {
		LOG_ERR("raw_pubkey_to_der failed! (Error: %d)", err);
		return -1;
	}

#if defined(CONFIG_APP_JWT_PRINT_EXPORTED_PUBKEY_DER)
	/* String size is double the binary size +1 for null termination */
	char pubkey_der_str[(ECDSA_PUBLIC_KEY_DER_SZ * 2) + 1] = {0};

	size_t pubkey_der_str_len = 0;

	pubkey_der_str_len = bin2hex(pubkey_der, pubkey_der_len, pubkey_der_str,
				sizeof(pubkey_der_str));
	LOG_INF("pubkey_der (%d) =  %s", pubkey_der_len, pubkey_der_str);

#endif /* CONFIG_APP_JWT_PRINT_EXPORTED_PUBKEY_DER */

	/* Compute the SHA256 hash of public key DER format */
	status = psa_hash_compute(PSA_ALG_SHA_256, pubkey_der, sizeof(pubkey_der), key_hash_out,
				  key_hash_buffer_size, key_hash_Length);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_hash_compute failed! (Error: %d)", status);
		return -1;
	}

	return status;
}

static int sign_message(const uint32_t user_key_id, const uint8_t *input, size_t input_length,
			uint8_t *signature, size_t signature_size, size_t *signature_length)
{
	psa_status_t status;

	/* Sign the hash */
	status = psa_sign_message(user_key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), input, input_length,
				  signature, signature_size, signature_length);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_sign_hash failed! (Error: %d)", status);
		return -1;
	}

	return 0;
}

#if defined(CONFIG_APP_JWT_VERIFY_SIGNATURE)

static int verify_message_signature(const uint32_t user_key_id, const char *const message,
				    size_t message_size, uint8_t *signature, size_t signature_size)
{
	psa_status_t status;

	/* Verify the signature of the message */
	status = psa_verify_message(user_key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), message,
				    message_size, signature, signature_size);
	if (status != PSA_SUCCESS) {
		LOG_ERR("signature verification failed! (Error: %d)", status);
		return -1;
	}

	return 0;
}
#endif /* CONFIG_APP_JWT_VERIFY_SIGNATURE */

static int get_random_bytes(uint8_t *output_buffer, const size_t output_length)
{
	int err = -ENODATA;

	if ((NULL == output_buffer) || (output_length < 4)) {
		/* Bad parameter */
		return -EINVAL;
	}

	psa_status_t psa_status = psa_generate_random(output_buffer, output_length);

	if (PSA_SUCCESS != psa_status) {
		LOG_ERR("psa_generate_random failed! = %d", psa_status);
		err = -ENOMEM;
	} else {
		err = 0;
	}

	return err;
}

static int crypto_finish(void)
{
	psa_status_t status;

	/* Purge the key from memory */
	status = psa_purge_key(IAK_APPLICATION_GEN1);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_purge_key failed! (Error: %d)", status);
		return -ENOMEM;
	}

	return 0;
}

static char *jwt_header_create(const uint32_t alg, const uint32_t keyid)
{
	int err = 0;

	uint8_t pub_key_hash[ECDSA_SHA_256_HASH_SZ];

	char *hdr_str = NULL;

	cJSON *jwt_hdr = cJSON_CreateObject();

	if (!jwt_hdr) {
		LOG_ERR("cJSON_CreateObject failed!");
		return NULL;
	}

	/* Type: format: always "JWT" */
	if (cJSON_AddStringToObjectCS(jwt_hdr, "typ", "JWT") == NULL) {
		goto clean_exit;
	}

	/* Algorithme: format: always "ES256" */
	if (alg == JWT_ALG_TYPE_ES256) {
		if (cJSON_AddStringToObjectCS(jwt_hdr, "alg", "ES256") == NULL) {
			goto clean_exit;
		}
	} else {
		goto clean_exit;
	}

	/* Keyid: format: sha256 string */
	/* Get kid: sha256 over public key */
	size_t olen;

	err = export_public_key_hash(IAK_APPLICATION_GEN1, pub_key_hash, ECDSA_SHA_256_HASH_SZ,
				     &olen);
	if (err) {
		LOG_ERR("Failed to export public key, error: %d", err);
		goto clean_exit;
	}

	char sha256_str[ECDSA_SHA_256_SIG_SZ + 1] = {0};

	int32_t printed_bytes = 0;

	for (uint32_t i = 0; i < 8; i++) {
		printed_bytes +=
			sprintf((sha256_str + printed_bytes), "%08x",
				sys_cpu_to_be32((uint32_t)*((uint32_t *)pub_key_hash + i)));
	}
	sha256_str[ECDSA_SHA_256_SIG_SZ] = '\0';

	if (cJSON_AddStringToObjectCS(jwt_hdr, "kid", sha256_str) == NULL) {
		err = -ENOMEM;
	}

	if (err == 0) {
		hdr_str = cJSON_PrintUnformatted(jwt_hdr);
	}

clean_exit:
	cJSON_Delete(jwt_hdr);
	jwt_hdr = NULL;

	return hdr_str;
}

static char *convert_str_to_b64_url(const char *const str)
{
	size_t str_len = strlen(str);

	size_t b64_sz = BASE64_ENCODE_SZ(str_len) + 1;

	char *const b64_out = calloc(b64_sz, 1);

	if (b64_out) {
		int err = base64_encode(b64_out, b64_sz, &b64_sz, str, strlen(str));

		if (err) {
			LOG_ERR("base64_encode failed, error: %d", err);

			free(b64_out);
			return NULL;
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

	char *msg_out = calloc(msg_sz, 1);

	/* Build the base64 URL JWT to sign:
	 * <base64_header>.<base64_payload>
	 */
	if (msg_out) {
		int ret = snprintk(msg_out, msg_sz, "%s.%s", hdr, pay);

		if ((ret < 0) || (ret >= msg_sz)) {
			LOG_ERR("Could not format JWT to be signed");
			free(msg_out);
			msg_out = NULL;
		}
	}

	return msg_out;
}

static char *jwt_payload_create(const char *const sub, const char *const aud, uint64_t exp)
{
	int err = 0;

	struct timespec tp;

	uint64_t issue_time;

	char *pay_str = NULL;

	cJSON *jwt_pay = cJSON_CreateObject();

	if (!jwt_pay) {
		LOG_ERR("cJSON_CreateObject failed!");
		return NULL;
	}

	err = clock_gettime(CLOCK_REALTIME, &tp);
	if (err) {
		/* clock_gettime error, use 0 value */
		issue_time = 0;
	} else {
		issue_time = tp.tv_sec;
	}

	/* Issued at : timestamp is seconds */
	if (cJSON_AddNumberToObjectCS(jwt_pay, "iat", issue_time) == NULL) {
		err = -ENOMEM;
	}

	/* Json Token ID: format: <hardware_id>.<16-random_bytes> */
	uint32_t nonce_words[JWT_NONCE_BITS / 32] = {0};

	err = get_random_bytes((uint8_t *)nonce_words, JWT_NONCE_BITS / 8);
	if (err) {
		LOG_ERR("get_random_bytes failed! = %d", err);
		err = -ENOMEM;
	} else {
		char nonce_uuid_str[APP_JWT_UUID_V4_STR_LEN] = {0};

		bytes_to_uuid_str(nonce_words, UUID_BINARY_WORD_SZ * sizeof(uint32_t),
				  nonce_uuid_str, APP_JWT_UUID_V4_STR_LEN);

		char jti_str[JWT_CLAIM_FILED_STR_LENGTH] = {0};

		int jti_str_length = sprintf(jti_str, "%s.%s", CONFIG_BOARD, nonce_uuid_str);

		jti_str[jti_str_length + 1] = '\0';
		if (cJSON_AddStringToObjectCS(jwt_pay, "jti", jti_str) == NULL) {
			err = -ENOMEM;
		}
	}

	if (sub != NULL) {
		/* Issuer: format: <hardware_id>.<sub> */
		char iss_str[JWT_CLAIM_FILED_STR_LENGTH] = {0};

		int iss_str_length = sprintf(iss_str, "%s.%s", CONFIG_BOARD, sub);

		iss_str[iss_str_length + 1] = '\0';
		if (cJSON_AddStringToObjectCS(jwt_pay, "iss", iss_str) == NULL) {
			err = -ENOMEM;
		}

		/* Subject: format: "user_defined_string" */
		if (cJSON_AddStringToObjectCS(jwt_pay, "sub", sub) == NULL) {
			err = -ENOMEM;
		}
	}

	/* Audience: format: "user_defined_string" */
	if (aud && (cJSON_AddStringToObjectCS(jwt_pay, "aud", aud) == NULL)) {
		err = -ENOMEM;
	}

	/* Expiration: format: time in seconds as integer + expiration */
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

static char *unsigned_jwt_create(struct app_jwt_data *const jwt)
{
	uint64_t exp_ts_s = 0;

	char *hdr_str, *pay_str;

	char *hdr_b64, *pay_b64;

	char *unsigned_jwt = NULL;

	struct timespec tp;

	/* Create the header */
	hdr_str = jwt_header_create(jwt->alg, jwt->key);
	if (!hdr_str) {
		LOG_ERR("Failed to create JWT JSON payload");
		return NULL;
	}

	/* Convert header JSON string to base64 URL */
	hdr_b64 = convert_str_to_b64_url(hdr_str);

	cJSON_free(hdr_str);
	hdr_str = NULL;

	if (!hdr_b64) {
		LOG_ERR("Failed to convert header string to base64");
		return NULL;
	}

	/* Get expiration time stamp */
	if (jwt->validity_s > 0) {
		int err = clock_gettime(CLOCK_REALTIME, &tp);

		if (err) {
			/* clock_gettime error, use 0 value */
			exp_ts_s = 0;
		} else {
			exp_ts_s = tp.tv_sec;
		}

		exp_ts_s += jwt->validity_s;
	}

	/* Create the payload */
	pay_str = jwt_payload_create(jwt->subject, jwt->audience, exp_ts_s);
	if (!pay_str) {
		LOG_ERR("Failed to create JWT JSON payload");
		goto clean_hdr_64_exit;
	}

	/* Convert payload JSON string to base64 URL */
	pay_b64 = convert_str_to_b64_url(pay_str);

	cJSON_free(pay_str);
	pay_str = NULL;

	if (!pay_b64) {
		LOG_ERR("Failed to convert payload string to base64");
		goto clean_hdr_64_exit;
	}

	/* Create the base64 URL data to be signed */
	unsigned_jwt = jwt_header_payload_combine(hdr_b64, pay_b64);

	free(pay_b64);
	pay_b64 = NULL;

clean_hdr_64_exit:
	free(hdr_b64);
	hdr_b64 = NULL;

	return unsigned_jwt;
}

static int jwt_signature_get(const uint32_t user_key_id, const int sec_tag, const char *const jwt,
			     char *const sig_buf, size_t sig_sz)
{
	int err;

	size_t o_len;

	uint8_t sig_raw[ECDSA_SHA_256_SIG_SZ];

	/* Use Application IAK key for signing the JWT */
	/* Ignore the provided key_id and sec_tag */
	(void)sec_tag;
	(void)user_key_id;
	err = sign_message(IAK_APPLICATION_GEN1, jwt, strlen(jwt), sig_raw, ECDSA_SHA_256_SIG_SZ,
			   &o_len);
	if (err) {
		LOG_ERR("Failed to sign message : %d", err);
		return -EACCES;
	}

#if defined(CONFIG_APP_JWT_VERIFY_SIGNATURE)
	err = verify_message_signature(IAK_APPLICATION_GEN1, jwt, strlen(jwt), sig_raw, o_len);
	if (err) {
		LOG_ERR("Failed to verify message signature : %d", err);
		return -EACCES;
	}
#endif /* CONFIG_APP_JWT_VERIFY_SIGNATURE */

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

	/* Get the size of the final, signed JWT: +1 for */
	/* '.' and null-terminator */
	size_t final_sz = strlen(unsigned_jwt) + 1 + strlen(sig) + 1;

	if (final_sz > jwt_sz) {
		/* Provided buffer is too small */
		return -E2BIG;
	}

	/* JWT final form:
	 * <base64_header>.<base64_payload>.<base64_signature>
	 */
	int ret = snprintk(jwt_buf, jwt_sz, "%s.%s", unsigned_jwt, sig);

	if ((ret < 0) || (ret >= jwt_sz)) {
		err = -ETXTBSY;
	}

	return err;
}

int app_jwt_generate(struct app_jwt_data *const jwt)
{
	if (jwt == NULL) {
		return -EINVAL;
	}

	if ((jwt->jwt_buf == NULL) || (jwt->jwt_sz == 0)) {
		return -EMSGSIZE;
	}

	int err = 0;

	char *unsigned_jwt;

	uint8_t jwt_sig[B64_SIG_SZ];

	/* Init crypto services, required for the rest of the operations */
	err = crypto_init();
	if (err) {
		LOG_ERR("Failed to initialize PSA Crypto, error: %d", err);
		return err;
	}

	/* Create the JWT to be signed */
	unsigned_jwt = unsigned_jwt_create(jwt);
	if (!unsigned_jwt) {
		LOG_ERR("Failed to create JWT to be signed");
		return -EIO;
	}

	/* Get the signature of the unsigned JWT */
	err = jwt_signature_get(jwt->key, jwt->sec_tag, unsigned_jwt, jwt_sig, sizeof(jwt_sig));
	if (err) {
		LOG_ERR("Failed to get JWT signature, error: %d", err);
		free(unsigned_jwt);
		unsigned_jwt = NULL;
		return -ENOEXEC;
	}

	/* Crypto services not required anymore */
	err = crypto_finish();
	if (err) {
		LOG_ERR("Failed to sign message : %d", err);
		return err;
	}

	/* Append the signature, creating the complete JWT */
	err = jwt_signature_append(unsigned_jwt, jwt_sig, jwt->jwt_buf, jwt->jwt_sz);

	free(unsigned_jwt);
	unsigned_jwt = NULL;

	return err;
}

int app_jwt_get_uuid(char *uuid_buffer, const size_t uuid_buffer_size)
{
	if ((NULL == uuid_buffer) || (uuid_buffer_size < APP_JWT_UUID_V4_STR_LEN)) {
		/* Bad parameter */
		return -EINVAL;
	}

	uint8_t uuid_bytes[UUID_BINARY_BYTES_SZ];

	if (0 != ssf_device_info_get_uuid(uuid_bytes)) {
		/* Couldn't read data */
		return -ENXIO;
	}

	return bytes_to_uuid_str((uint32_t *)uuid_bytes, UUID_BINARY_BYTES_SZ, uuid_buffer,
				 uuid_buffer_size);
}
