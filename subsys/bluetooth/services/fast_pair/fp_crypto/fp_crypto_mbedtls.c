/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include "fp_crypto.h"

#include <mbedtls/aes.h>
#include <mbedtls/bignum.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecp.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>

#include <zephyr/random/random.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_crypto, CONFIG_FP_CRYPTO_LOG_LEVEL);

/* Bit length of the AES-128 key. */
#define AES128_ECB_KEY_BIT_LEN (128)
/* Byte length of the coordinate point on the SECP256R1 elliptic curve. */
#define SECP256R1_POINT_COORDINATE_LEN (32)
/* Marker of the uncompressed binary format for a point on an elliptic curve. */
#define UNCOMPRESSED_FORMAT_MARKER (0x04)
/* Byte length of the uncompressed binary format marker. */
#define UNCOMPRESSED_FORMAT_MARKER_LEN (1)
/* Byte length of the point on the SECP256R1 elliptic curve in the uncompressed binary format. */
#define SECP256R1_UNCOMPRESSED_POINT_LEN \
	(2 * SECP256R1_POINT_COORDINATE_LEN + UNCOMPRESSED_FORMAT_MARKER_LEN)

typedef int (*mbedtls_aes_setkey)(mbedtls_aes_context *, const unsigned char *, unsigned int);

static int random_data_generate(void *context, unsigned char *data, size_t datalen)
{
	ARG_UNUSED(context);

	int ret;

	ret = sys_csrand_get(data, datalen);
	if (ret) {
		LOG_ERR("rng: sys_csrand_get failed: %d", ret);
		return ret;
	}

	return 0;
}

int fp_crypto_sha256(uint8_t *out, const uint8_t *in, size_t data_len)
{
	int ret;
	const int is_sha224 = 0;
	mbedtls_sha256_context sha256_context;

	mbedtls_sha256_init(&sha256_context);

	ret = mbedtls_sha256_starts(&sha256_context, is_sha224);
	if (ret) {
		LOG_ERR("sha256: mbedtls_sha256_starts failed: %d", ret);
		goto cleanup;
	}

	ret = mbedtls_sha256_update(&sha256_context, in, data_len);
	if (ret) {
		LOG_ERR("sha256: mbedtls_sha256_update failed: %d", ret);
		goto cleanup;
	}

	ret = mbedtls_sha256_finish(&sha256_context, out);
	if (ret) {
		LOG_ERR("sha256: mbedtls_sha256_finish failed: %d", ret);
		goto cleanup;
	}

cleanup:
	/* Free the SHA256 context. */
	mbedtls_sha256_free(&sha256_context);

	return ret;
}

int fp_crypto_hmac_sha256(uint8_t *out,
			  const uint8_t *in,
			  size_t data_len,
			  const uint8_t *hmac_key,
			  size_t hmac_key_len)
{
	int ret;
	const mbedtls_md_info_t *md_info;

	md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (!md_info) {
		LOG_ERR("hmac sha256: message-digest information not found");
		return MBEDTLS_ERR_MD_FEATURE_UNAVAILABLE;
	}

	ret = mbedtls_md_hmac(md_info, hmac_key, hmac_key_len, in, data_len, out);
	if (ret) {
		LOG_ERR("hmac sha256: mbedtls_md_hmac failed: %d", ret);
		return ret;
	}

	return 0;
}

static int aes128_ecb_crypt(uint8_t *out, const uint8_t *in, const uint8_t *k, bool encrypt)
{
	int ret;
	const int mode = encrypt ? MBEDTLS_AES_ENCRYPT : MBEDTLS_AES_DECRYPT;
	const char *log_prefix = encrypt ? "aes128_ecb_encrypt" : "aes128_ecb_decrypt";
	mbedtls_aes_setkey mbedtls_aes_setkey = encrypt ?
		mbedtls_aes_setkey_enc : mbedtls_aes_setkey_dec;
	mbedtls_aes_context aes_ctx;

	mbedtls_aes_init(&aes_ctx);

	ret = mbedtls_aes_setkey(&aes_ctx, k, AES128_ECB_KEY_BIT_LEN);
	if (ret) {
		LOG_ERR("%s: mbedtls_aes_setkey_enc failed: %d", log_prefix, ret);
		goto cleanup;
	}

	ret = mbedtls_aes_crypt_ecb(&aes_ctx, mode, in, out);
	if (ret) {
		LOG_ERR("%s: mbedtls_aes_crypt_ecb for 0..15 failed: %d", log_prefix, ret);
		goto cleanup;
	}

cleanup:
	/* Free the AES context. */
	mbedtls_aes_free(&aes_ctx);

	return ret;
}

int fp_crypto_aes128_ecb_encrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	return aes128_ecb_crypt(out, in, k, true);
}

int fp_crypto_aes128_ecb_decrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	return aes128_ecb_crypt(out, in, k, false);
}

int fp_crypto_ecdh_shared_secret(uint8_t *secret_key,
				 const uint8_t *public_key,
				 const uint8_t *private_key)
{
	int ret;
	uint8_t public_key_uncompressed[SECP256R1_UNCOMPRESSED_POINT_LEN];
	mbedtls_mpi shared_secret_mpi;
	mbedtls_mpi private_key_mpi;
	mbedtls_ecp_point public_key_point;
	mbedtls_ecp_group secp256r1;

	mbedtls_mpi_init(&shared_secret_mpi);
	mbedtls_mpi_init(&private_key_mpi);
	mbedtls_ecp_point_init(&public_key_point);
	mbedtls_ecp_group_init(&secp256r1);

	ret = mbedtls_ecp_group_load(&secp256r1, MBEDTLS_ECP_DP_SECP256R1);
	if (ret) {
		LOG_ERR("ecdh: mbedtls_ecp_group_load failed: %d", ret);
		goto cleanup;
	}

	/* Use the uncompressed binary format [0x04 X Y] for the public key point. */
	public_key_uncompressed[0] = UNCOMPRESSED_FORMAT_MARKER;
	memcpy(&public_key_uncompressed[UNCOMPRESSED_FORMAT_MARKER_LEN],
	       public_key,
	       sizeof(public_key_uncompressed) - UNCOMPRESSED_FORMAT_MARKER_LEN);
	ret = mbedtls_ecp_point_read_binary(&secp256r1, &public_key_point,
					    public_key_uncompressed,
					    sizeof(public_key_uncompressed));
	if (ret) {
		LOG_ERR("ecdh: mbedtls_ecp_point_read_binary failed: %d", ret);
		goto cleanup;
	}

	ret = mbedtls_mpi_read_binary(&private_key_mpi,
				      private_key,
				      SECP256R1_POINT_COORDINATE_LEN);
	if (ret) {
		LOG_ERR("ecdh: mbedtls_mpi_read_binary failed: %d", ret);
		goto cleanup;
	}

	ret = mbedtls_ecdh_compute_shared(&secp256r1,
					  &shared_secret_mpi,
					  &public_key_point,
					  &private_key_mpi,
					  random_data_generate,
					  NULL);
	if (ret) {
		LOG_ERR("ecdh: mbedtls_ecdh_compute_shared failed: %d", ret);
		goto cleanup;
	}

	ret = mbedtls_mpi_write_binary(&shared_secret_mpi, secret_key,
				       FP_CRYPTO_ECDH_SHARED_KEY_LEN);
	if (ret) {
		LOG_ERR("ecdh: mbedtls_mpi_write_binary failed: %d", ret);
		goto cleanup;
	}

cleanup:
	/* Free initialized contexts. */
	mbedtls_mpi_free(&shared_secret_mpi);
	mbedtls_mpi_free(&private_key_mpi);
	mbedtls_ecp_point_free(&public_key_point);
	mbedtls_ecp_group_free(&secp256r1);

	return ret;
}
