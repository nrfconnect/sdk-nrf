/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/init.h>
#include <psa/crypto.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_crypto, CONFIG_FP_CRYPTO_LOG_LEVEL);

#include "fp_crypto.h"

int fp_crypto_sha256(uint8_t *out, const uint8_t *in, size_t data_len)
{
	size_t hash_len = 0;
	psa_status_t status = psa_hash_compute(PSA_ALG_SHA_256, in, data_len,
					       out, FP_CRYPTO_SHA256_HASH_LEN, &hash_len);

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_hash_compute failed (err: %d)", status);
		return -EIO;
	}

	if (hash_len != FP_CRYPTO_SHA256_HASH_LEN) {
		LOG_ERR("Invalid psa_hash_compute output len: %zu", hash_len);
		return -EIO;
	}

	return 0;
}

static psa_key_id_t import_hmac_sha256_key(const uint8_t *data, size_t len)
{
	psa_status_t status;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_lifetime(&key_attr, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attr, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&key_attr, len * CHAR_BIT);

	status = psa_import_key(&key_attr, data, len, &key_id);
	psa_reset_key_attributes(&key_attr);

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_import_key failed (err: %d)", status);
		key_id = PSA_KEY_ID_NULL;
	}

	return key_id;
}

static int fp_crypto_psa_hmac_sha256(uint8_t *out, const uint8_t *in, size_t data_len,
				     psa_key_id_t key_id)
{
	size_t olen = 0;
	psa_status_t status = psa_mac_compute(key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256), in, data_len,
					      out, FP_CRYPTO_SHA256_HASH_LEN, &olen);

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_mac_compute failed (err: %d)", status);
		return -EIO;
	}

	if (olen != FP_CRYPTO_SHA256_HASH_LEN) {
		LOG_ERR("Invalid psa_mac_compute output length: %zu", olen);
		return -EIO;
	}

	return 0;
}

int fp_crypto_hmac_sha256(uint8_t *out, const uint8_t *in, size_t data_len, const uint8_t *hmac_key,
			  size_t hmac_key_len)
{
	int err = 0;
	psa_key_id_t hmac_key_id;
	psa_status_t status;

	hmac_key_id = import_hmac_sha256_key(hmac_key, hmac_key_len);
	if (hmac_key_id == PSA_KEY_ID_NULL) {
		LOG_ERR("import_hmac_sha256_key failed");
		return -EIO;
	}

	err = fp_crypto_psa_hmac_sha256(out, in, data_len, hmac_key_id);

	status = psa_destroy_key(hmac_key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_destroy_key failed (err: %d)", status);
		/* Overwrite error code to forward information about psa_destroy_key failure. */
		err = -ECANCELED;
	}

	return err;
}

static psa_key_id_t import_aes128_key(const uint8_t *data)
{
	static const size_t len = FP_CRYPTO_AES128_KEY_LEN;

	psa_status_t status;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attr, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attr, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&key_attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attr, len * CHAR_BIT);

	status = psa_import_key(&key_attr, data, len, &key_id);
	psa_reset_key_attributes(&key_attr);

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_import_key failed (err: %d)", status);
		key_id = PSA_KEY_ID_NULL;
	}

	return key_id;
}

static int fp_crypto_psa_aes128_ecb_crypt(uint8_t *out, const uint8_t *in, psa_key_id_t key_id,
					  bool encrypt)
{
	size_t olen = 0;
	psa_status_t status;

	if (encrypt) {
		status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING,
					    in, FP_CRYPTO_AES128_BLOCK_LEN,
					    out, FP_CRYPTO_AES128_BLOCK_LEN, &olen);
	} else {
		status = psa_cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING,
					    in, FP_CRYPTO_AES128_BLOCK_LEN,
					    out, FP_CRYPTO_AES128_BLOCK_LEN, &olen);
	}

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_%scrypt failed (err: %d)", encrypt ? "en" : "de", status);
		return -EIO;
	}

	if (olen != FP_CRYPTO_AES128_BLOCK_LEN) {
		LOG_ERR("Invalid psa_cipher_%scrypt output length: %zu",
			encrypt ? "en" : "de", olen);
		return -EIO;
	}

	return 0;
}

static int fp_crypto_aes128_ecb_crypt(uint8_t *out, const uint8_t *in, const uint8_t *k,
				      bool encrypt)
{
	int err = 0;
	psa_key_id_t key_id;
	psa_status_t status;

	key_id = import_aes128_key(k);
	if (key_id == PSA_KEY_ID_NULL) {
		LOG_ERR("import_aes128_key failed");
		return -EIO;
	}

	err = fp_crypto_psa_aes128_ecb_crypt(out, in, key_id, encrypt);

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_destroy_key failed (err: %d)", status);
		/* Overwrite error code to forward information about psa_destroy_key failure. */
		err = -ECANCELED;
	}

	return err;
}

int fp_crypto_aes128_ecb_encrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	return fp_crypto_aes128_ecb_crypt(out, in, k, true);
}

int fp_crypto_aes128_ecb_decrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	return fp_crypto_aes128_ecb_crypt(out, in, k, false);
}

static psa_key_id_t import_ecdh_priv_key(const uint8_t *data)
{
	static const size_t len = 32;

	psa_status_t status;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&key_attr, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attr, PSA_ALG_ECDH);
	psa_set_key_type(&key_attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attr, len * CHAR_BIT);

	status = psa_import_key(&key_attr, data, len, &key_id);
	psa_reset_key_attributes(&key_attr);

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_import_key failed (err: %d)", status);
		key_id = PSA_KEY_ID_NULL;
	}

	return key_id;
}

int fp_crypto_psa_ecdh_shared_secret(uint8_t *secret_key, const uint8_t *public_key,
				     psa_key_handle_t priv_key_id)
{
	/* Marker of the uncompressed binary format for a point on an elliptic curve. */
	static const uint8_t uncompressed_format_marker = 0x04;

	uint8_t public_key_uncompressed[sizeof(uncompressed_format_marker) +
					FP_CRYPTO_ECDH_PUBLIC_KEY_LEN];
	size_t olen = 0;
	psa_status_t status;

	/* Use the uncompressed binary format [0x04 X Y] for the public key point. */
	public_key_uncompressed[0] = uncompressed_format_marker;
	memcpy(&public_key_uncompressed[1], public_key, FP_CRYPTO_ECDH_PUBLIC_KEY_LEN);

	status = psa_raw_key_agreement(PSA_ALG_ECDH, priv_key_id,
				       public_key_uncompressed, sizeof(public_key_uncompressed),
				       secret_key, FP_CRYPTO_ECDH_SHARED_KEY_LEN, &olen);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_raw_key_agreement failed (err: %d)", status);
		return -EIO;
	}

	if (olen != FP_CRYPTO_ECDH_SHARED_KEY_LEN) {
		LOG_ERR("Invalid psa_raw_key_agreement output len: %zu", olen);
		return -EIO;
	}

	return 0;
}

int fp_crypto_ecdh_shared_secret(uint8_t *secret_key, const uint8_t *public_key,
				 const uint8_t *private_key)
{

	int err = 0;
	psa_key_handle_t priv_key_id;
	psa_status_t status;

	priv_key_id = import_ecdh_priv_key(private_key);
	if (priv_key_id == PSA_KEY_ID_NULL) {
		LOG_ERR("import_ecdh_shared_secret_key failed");
		return -EIO;
	}

	err = fp_crypto_psa_ecdh_shared_secret(secret_key, public_key, priv_key_id);

	status = psa_destroy_key(priv_key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_destroy_key failed (err: %d)", status);
		/* Overwrite error code to forward information about psa_destroy_key failure. */
		err = -ECANCELED;
	}

	return err;
}

static int fp_crypto_psa_init(void)
{
	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed (err: %d)", status);
		k_panic();
		return -EIO;
	}

	return 0;
}

SYS_INIT(fp_crypto_psa_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
