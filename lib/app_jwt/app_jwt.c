/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
/* Define _POSIX_C_SOURCE before including <string.h> in order to use `strtok_r`. */
#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <date_time.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#if defined(CONFIG_BOARD_NATIVE_SIM)
#define IAK_APPLICATION_GEN1 0x41020100
#else
#include <psa/nrf_platform_key_ids.h>
#endif

#include <cJSON.h>

#include <app_jwt.h>

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

/* Default signing key is IAK GEN1 */
#define DEFAULT_IAK_APPLICATION IAK_APPLICATION_GEN1

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

/* Maximum possible size for a JWT in a string format */
#define JWT_STR_MAX_SZ (900)

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
	/* (string Length 8 + 1) */
	char temp_str[UUID_BINARY_WORD_SZ][BINARY_WORD_STR_SZ] = {0};

	/* Transform integers to strings */
	for (int i = 0; i < UUID_BINARY_WORD_SZ; i++) {
		/* UUID byte endiannes is little endian first. */
		snprintf(temp_str[i], 9, "%08x", sys_cpu_to_be32(uuid_words[i]));
	}

	/* UUID string format defined by  ITU-T X.667 | ISO/IEC 9834-8 : */
	/* <8 char>-<4 char>-<4 char>-<4 char>-<12 char> */

	snprintf(uuid_str_out, APP_JWT_UUID_V4_STR_LEN, "%.8s-%.4s-%.4s-%.4s-%.4s%.8s", temp_str[0],
		 temp_str[1] + 0, temp_str[1] + 4, temp_str[2] + 0, temp_str[2] + 4, temp_str[3]);

	uuid_str_out[APP_JWT_UUID_V4_STR_LEN - 1] = '\0';

	return 0;
}

static int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init() failed! (Error: %d)", status);
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
		LOG_ERR("psa_export_public_key() failed! (Error: %d)", status);
		return -1;
	}

	uint8_t pubkey_der[ECDSA_PUBLIC_KEY_DER_SZ] = {0};
	size_t pubkey_der_len = 0;

	err = raw_ecc_pubkey_to_der(pub_key, pubkey_der, sizeof(pubkey_der), &pubkey_der_len);
	if (err != 0) {
		LOG_ERR("raw_pubkey_to_der() failed! (Error: %d)", err);
		return -1;
	}

#if defined(CONFIG_APP_JWT_PRINT_EXPORTED_PUBKEY_DER)
	/* String size is double the binary size +1 for null termination */
	char pubkey_der_str[(ECDSA_PUBLIC_KEY_DER_SZ * 2) + 1] = {0};

	size_t pubkey_der_str_len = 0;

	pubkey_der_str_len =
		bin2hex(pubkey_der, pubkey_der_len, pubkey_der_str, sizeof(pubkey_der_str));
	LOG_INF("pubkey_der (%d bytes) =  %s", pubkey_der_len, pubkey_der_str);

#endif /* CONFIG_APP_JWT_PRINT_EXPORTED_PUBKEY_DER */

	/* Compute the SHA256 hash of public key DER format */
	status = psa_hash_compute(PSA_ALG_SHA_256, pubkey_der, sizeof(pubkey_der), key_hash_out,
				  key_hash_buffer_size, key_hash_Length);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_hash_compute() failed! (Error: %d)", status);
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
		LOG_ERR("psa_sign_message() failed! (Error: %d)", status);
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
		LOG_ERR("psa_verify_message() failed! (Error: %d)", status);
		return -1;
	}

	return 0;
}
#endif /* CONFIG_APP_JWT_VERIFY_SIGNATURE */

static int crypto_finish(const int key_id)
{
	/**
	 * Purge any IAK copies from memory.
	 * A user private key has to be handled by its owner.
	 */
	if (DEFAULT_IAK_APPLICATION == key_id) {
		psa_status_t status;

		status = psa_purge_key(key_id);
		if (status != PSA_SUCCESS) {
			LOG_DBG("psa_purge_key() failed! (Error: %d)", status);
			return -ENOMEM;
		}
	}

	return 0;
}

static char *jwt_header_create(struct app_jwt_data *const jwt)
{
	if (jwt == NULL) {
		return NULL;
	}

	int err = 0;
	char *hdr_str = NULL;
	int key_id = DEFAULT_IAK_APPLICATION;

	cJSON *jwt_hdr = cJSON_CreateObject();

	if (!jwt_hdr) {
		LOG_ERR("cJSON_CreateObject() failed!");
		return NULL;
	}

	if (jwt->sec_tag != 0) {
		/**
		 * Using sec_tag as key_id, you should be sure the provided
		 * sec_tag is a valid key_id
		 */
		LOG_DBG("sec_tag provided, value = 0x%08X", jwt->sec_tag);
		key_id = jwt->sec_tag;
	} else {
		key_id = DEFAULT_IAK_APPLICATION;
	}

	/* Type: format: always "JWT" */
	if (cJSON_AddStringToObjectCS(jwt_hdr, "typ", "JWT") == NULL) {
		goto clean_exit;
	}

	/* Algorithme: format: always "ES256" */
	if (jwt->alg == JWT_ALG_TYPE_ES256) {
		if (cJSON_AddStringToObjectCS(jwt_hdr, "alg", "ES256") == NULL) {
			goto clean_exit;
		}
	} else {
		goto clean_exit;
	}

	uint8_t pub_key_hash[ECDSA_SHA_256_HASH_SZ];

	/* Kid: format: sha256 string */
	/* Get kid: sha256 over public key */
	size_t olen;

	err = export_public_key_hash(key_id, pub_key_hash, ECDSA_SHA_256_HASH_SZ, &olen);
	if (err) {
		LOG_ERR("export_public_key_hash() failed! (Error: %d)", err);
		goto clean_exit;
	}

	if (jwt->add_keyid_to_header) {
		char sha256_str[ECDSA_SHA_256_SIG_SZ + 1] = {0};

		int32_t printed_bytes = 0;

		for (uint32_t i = 0; i < 8; i++) {
			printed_bytes += snprintf(
				(sha256_str + printed_bytes), 9, "%08x",
				sys_cpu_to_be32((uint32_t)*((uint32_t *)pub_key_hash + i)));
		}
		sha256_str[ECDSA_SHA_256_SIG_SZ] = '\0';

		if (cJSON_AddStringToObjectCS(jwt_hdr, "kid", sha256_str) == NULL) {
			err = -ENOMEM;
		}
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
	if (str == NULL) {
		return NULL;
	}

	size_t str_len = strnlen(str, JWT_STR_MAX_SZ);
	size_t b64_sz = BASE64_ENCODE_SZ(str_len) + 1;
	char *const b64_out = calloc(b64_sz, 1);

	if (b64_out) {
		int err =
			base64_encode(b64_out, b64_sz, &b64_sz, str, strnlen(str, JWT_STR_MAX_SZ));

		if (err) {
			LOG_ERR("base64_encode() failed! (Error: %d)", err);

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
	if (hdr == NULL || pay == NULL) {
		return NULL;
	}
	/* Allocate buffer for the JWT header and payload to be signed */
	size_t msg_sz = strnlen(hdr, JWT_STR_MAX_SZ) + 1 + strnlen(pay, JWT_STR_MAX_SZ) + 1;
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

static char *jwt_payload_create(struct app_jwt_data *const jwt)
{
	if (jwt == NULL) {
		return NULL;
	}

	int err = 0;
	uint64_t unix_time_ms = 0;
	uint64_t issue_time = CONFIG_APP_JWT_DEFAULT_TIMESTAMP;
	char *pay_str = NULL;

	cJSON *jwt_pay = cJSON_CreateObject();

	if (!jwt_pay) {
		LOG_ERR("cJSON_CreateObject() failed!");
		return NULL;
	}

	err = date_time_now(&unix_time_ms);
	if (err) {
		/* date_time_now error, use DEFAULT_TIMESTAMP value */
		LOG_WRN("date_time_now() error, use %d", CONFIG_APP_JWT_DEFAULT_TIMESTAMP);
		err = 0;
	} else {
		issue_time = (unix_time_ms / 1000);
		if (issue_time == 0) {
			LOG_WRN("invalid time value, use %d", CONFIG_APP_JWT_DEFAULT_TIMESTAMP);
		}
	}

	if (jwt->add_timestamp && (issue_time != 0)) {
		/* Issued at : timestamp is seconds */
		if (cJSON_AddNumberToObjectCS(jwt_pay, "iat", issue_time) == NULL) {
			err = -ENOMEM;
		}
	}

	if (jwt->json_token_id != NULL) {
		char claim_str[APP_JWT_CLAIM_MAX_SIZE] = {0};

		snprintf(claim_str, APP_JWT_CLAIM_MAX_SIZE, "%s", jwt->json_token_id);
		claim_str[APP_JWT_CLAIM_MAX_SIZE - 1] = '\0';

		if (cJSON_AddStringToObjectCS(jwt_pay, "jti", claim_str) == NULL) {
			err = -ENOMEM;
		}
	}

	if (jwt->issuer != NULL) {
		char claim_str[APP_JWT_CLAIM_MAX_SIZE] = {0};

		snprintf(claim_str, APP_JWT_CLAIM_MAX_SIZE, "%s", jwt->issuer);
		claim_str[APP_JWT_CLAIM_MAX_SIZE - 1] = '\0';

		if (cJSON_AddStringToObjectCS(jwt_pay, "iss", claim_str) == NULL) {
			err = -ENOMEM;
		}
	}

	if (jwt->subject != NULL) {
		char claim_str[APP_JWT_CLAIM_MAX_SIZE] = {0};

		snprintf(claim_str, APP_JWT_CLAIM_MAX_SIZE, "%s", jwt->subject);
		claim_str[APP_JWT_CLAIM_MAX_SIZE - 1] = '\0';

		if (cJSON_AddStringToObjectCS(jwt_pay, "sub", claim_str) == NULL) {
			err = -ENOMEM;
		}
	}

	if (jwt->audience != NULL) {
		char claim_str[APP_JWT_CLAIM_MAX_SIZE] = {0};

		snprintf(claim_str, APP_JWT_CLAIM_MAX_SIZE, "%s", jwt->audience);
		claim_str[APP_JWT_CLAIM_MAX_SIZE - 1] = '\0';

		if (cJSON_AddStringToObjectCS(jwt_pay, "aud", claim_str) == NULL) {
			err = -ENOMEM;
		}
	}

	/* Expiration: format: time in seconds as integer + expiration */
	if (jwt->validity_s > 0) {
		/* Add expiration (exp) claim */
		if (cJSON_AddNumberToObjectCS(jwt_pay, "exp", (jwt->validity_s + issue_time)) ==
		    NULL) {
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
	char *hdr_str, *pay_str;
	char *hdr_b64, *pay_b64;
	char *unsigned_jwt = NULL;

	if (jwt == NULL) {
		return NULL;
	}

	/* Create the header */
	hdr_str = jwt_header_create(jwt);
	if (!hdr_str) {
		LOG_ERR("Failed to create JWT JSON header");
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

	/* Create the payload */
	pay_str = jwt_payload_create(jwt);
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

static int jwt_signature_get(const int key_id, const char *const jwt, char *const sig_buf,
			     size_t sig_sz)
{
	int err;
	size_t o_len;
	uint8_t sig_raw[ECDSA_SHA_256_SIG_SZ];

	if (jwt == NULL || sig_buf == NULL) {
		return -EINVAL;
	}

	/* Use Application IAK key for signing the JWT */
	err = sign_message(key_id, jwt, strnlen(jwt, JWT_STR_MAX_SZ), sig_raw, ECDSA_SHA_256_SIG_SZ,
			   &o_len);
	if (err) {
		LOG_ERR("sign_message() failed! (Error: %d)", err);
		return -EACCES;
	}

#if defined(CONFIG_APP_JWT_VERIFY_SIGNATURE)
	err = verify_message_signature(key_id, jwt, strnlen(jwt, JWT_STR_MAX_SZ), sig_raw, o_len);
	if (err) {
		LOG_ERR("verify_message_signature() failed! (Error: %d)", err);
		return -EACCES;
	}
#endif /* CONFIG_APP_JWT_VERIFY_SIGNATURE */

	/* Convert signature to base64 URL */
	err = base64_encode(sig_buf, sig_sz, &o_len, sig_raw, sizeof(sig_raw));
	if (err) {
		LOG_ERR("base64_encode() failed! (Error: %d)", err);
		return -EIO;
	}

	base64_url_format(sig_buf);
	return 0;
}

static int jwt_signature_append(const char *const unsigned_jwt, const char *const sig,
				char *const jwt_buf, size_t jwt_sz)
{
	int err = 0;

	if (unsigned_jwt == NULL || sig == NULL) {
		return -EINVAL;
	}

	/* Get the size of the final, signed JWT: +1 for */
	/* '.' and null-terminator */
	size_t final_sz =
		strnlen(unsigned_jwt, JWT_STR_MAX_SZ) + 1 + strnlen(sig, ECDSA_SHA_256_SIG_SZ) + 1;

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
	int key_id = 0;
	char *unsigned_jwt;
	uint8_t jwt_sig[B64_SIG_SZ];

	if (jwt->sec_tag != 0) {
		/**
		 * Using sec_tag as key_id, you should be sure the provided
		 * sec_tag is a valid key_id
		 */
		LOG_DBG("sec_tag provided, value = 0x%08X", jwt->sec_tag);
		key_id = jwt->sec_tag;
	} else {
		key_id = DEFAULT_IAK_APPLICATION;
	}

	/* Init crypto services, required for the rest of the operations */
	err = crypto_init();
	if (err) {
		LOG_ERR("crypto_init() failed! (Error: %d)", err);
		return err;
	}

	/* Create the JWT to be signed */
	unsigned_jwt = unsigned_jwt_create(jwt);
	if (!unsigned_jwt) {
		LOG_ERR("unsigned_jwt_create() failed!");
		err = -EIO;
		goto finish_crypto_exit;
	}

	/* Get the signature of the unsigned JWT */
	err = jwt_signature_get(key_id, unsigned_jwt, jwt_sig, sizeof(jwt_sig));
	if (err) {
		LOG_ERR("jwt_signature_get() failed! (Error: %d)", err);
		goto clean_exit;
	}

	/* Append the signature, creating the complete JWT */
	err = jwt_signature_append(unsigned_jwt, jwt_sig, jwt->jwt_buf, jwt->jwt_sz);
	if (err) {
		LOG_ERR("jwt_signature_append() failed! (Error: %d)", err);
	}

clean_exit:
	free(unsigned_jwt);
	unsigned_jwt = NULL;

finish_crypto_exit:
	/* Crypto services not required anymore */
	if (crypto_finish(key_id)) {
		LOG_DBG("crypto_finish() failed!");
	}

	return err;
}

int app_jwt_get_uuid(char *uuid_buffer, const size_t uuid_buffer_size)
{
	if ((NULL == uuid_buffer) || (uuid_buffer_size < APP_JWT_UUID_V4_STR_LEN)) {
		/* Bad parameter */
		return -EINVAL;
	}

	uint8_t uuid_bytes[UUID_BINARY_BYTES_SZ] = {0}; /* TODO NRFX-8297 */

	return bytes_to_uuid_str((uint32_t *)uuid_bytes, UUID_BINARY_BYTES_SZ, uuid_buffer,
				 uuid_buffer_size);
}
