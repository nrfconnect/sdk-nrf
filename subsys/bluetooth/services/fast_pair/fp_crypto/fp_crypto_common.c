/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <limits.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_crypto, CONFIG_FP_CRYPTO_LOG_LEVEL);

#include "fp_crypto.h"
#include "fp_common.h"

#define ADDITIONAL_DATA_SHA_POS		0
#define ADDITIONAL_DATA_SHA_LEN		8
#define ADDITIONAL_DATA_NONCE_POS	(ADDITIONAL_DATA_SHA_POS + ADDITIONAL_DATA_SHA_LEN)
#define ADDITIONAL_DATA_ENC_DATA_POS	(ADDITIONAL_DATA_NONCE_POS + \
					 FP_CRYPTO_ADDITIONAL_DATA_NONCE_LEN)


int fp_crypto_hmac_sha256(uint8_t *out, const uint8_t *in, size_t data_len, const uint8_t *aes_key)
{
	static const uint8_t hmac_sha_pad_len = 64;
	static const uint8_t inner_sha_data_pos = hmac_sha_pad_len;
	static const uint8_t hmac_sha_inner_sha_pos = hmac_sha_pad_len;
	static const uint8_t hmac_sha_opad_val = 0x5C;
	static const uint8_t hmac_sha_ipad_val = 0x36;

	uint8_t inner_sha_input[hmac_sha_pad_len + data_len];
	uint8_t sha_input[hmac_sha_pad_len + FP_CRYPTO_SHA256_HASH_LEN];
	int err;

	for (size_t i = 0; i < FP_CRYPTO_AES128_KEY_LEN; i++) {
		inner_sha_input[i] = aes_key[i] ^ hmac_sha_ipad_val;
	}

	memset(&inner_sha_input[FP_CRYPTO_AES128_KEY_LEN], hmac_sha_ipad_val,
	       hmac_sha_pad_len - FP_CRYPTO_AES128_KEY_LEN);
	memcpy(&inner_sha_input[inner_sha_data_pos], in, data_len);

	err = fp_crypto_sha256(&sha_input[hmac_sha_inner_sha_pos], inner_sha_input,
			       sizeof(inner_sha_input));
	if (err) {
		return err;
	}

	for (size_t i = 0; i < FP_CRYPTO_AES128_KEY_LEN; i++) {
		sha_input[i] = aes_key[i] ^ hmac_sha_opad_val;
	}

	memset(&sha_input[FP_CRYPTO_AES128_KEY_LEN], hmac_sha_opad_val,
	       hmac_sha_pad_len - FP_CRYPTO_AES128_KEY_LEN);

	return fp_crypto_sha256(out, sha_input, sizeof(sha_input));
}

int fp_crypto_aes128_ctr_encrypt(uint8_t *out, const uint8_t *in, size_t data_len,
				 const uint8_t *key, const uint8_t *nonce)
{
	static const uint8_t aes_input_pad_pos = 1;
	static const uint8_t aes_input_pad_len = 7;
	static const uint8_t aes_input_nonce_pos = aes_input_pad_pos + aes_input_pad_len;

	size_t block_cnt;
	uint8_t aes_input[FP_CRYPTO_AES128_BLOCK_LEN];
	uint8_t aes_block[FP_CRYPTO_AES128_BLOCK_LEN];
	int err;

	if ((data_len % FP_CRYPTO_AES128_BLOCK_LEN) == 0) {
		block_cnt = data_len / FP_CRYPTO_AES128_BLOCK_LEN;
	} else {
		block_cnt = (data_len / FP_CRYPTO_AES128_BLOCK_LEN) + 1;
	}

	memset(&aes_input[aes_input_pad_pos], 0, aes_input_pad_len);
	memcpy(&aes_input[aes_input_nonce_pos], nonce, FP_CRYPTO_ADDITIONAL_DATA_NONCE_LEN);

	memcpy(out, in, data_len);

	for (size_t i = 0; i < block_cnt; i++) {
		aes_input[0] = (uint8_t)i;

		err = fp_crypto_aes128_encrypt(aes_block, aes_input, key);
		if (err) {
			return err;
		}

		for (size_t j = 0; (j < FP_CRYPTO_AES128_BLOCK_LEN) &&
				   (i * FP_CRYPTO_AES128_BLOCK_LEN + j < data_len); j++) {
			out[i * FP_CRYPTO_AES128_BLOCK_LEN + j] ^= aes_block[j];
		}
	}

	return 0;
}

int fp_crypto_aes128_ctr_decrypt(uint8_t *out, const uint8_t *in, size_t data_len,
				 const uint8_t *key, const uint8_t *nonce)
{
	/* Due to properties of XOR function data encrypted twice is the same as initial data, so to
	 * decrypt data it is sufficient to encrypt encrypted data.
	 */
	return fp_crypto_aes128_ctr_encrypt(out, in, data_len, key, nonce);
}

int fp_crypto_aes_key_compute(uint8_t *out, const uint8_t *in)
{
	uint8_t hashed_key_buf[FP_CRYPTO_SHA256_HASH_LEN];
	int err = fp_crypto_sha256(hashed_key_buf, in, FP_CRYPTO_ECDH_SHARED_KEY_LEN);

	if (err) {
		return err;
	}
	memcpy(out, hashed_key_buf, FP_CRYPTO_AES128_KEY_LEN);
	return 0;
}

size_t fp_crypto_account_key_filter_size(size_t n)
{
	if (n == 0) {
		return 0;
	} else {
		return 1.2 * n + 3;
	}
}

int fp_crypto_account_key_filter(uint8_t *out, const struct fp_account_key *account_key_list,
				 size_t n, uint8_t salt)
{
	size_t s = fp_crypto_account_key_filter_size(n);
	uint8_t v[FP_ACCOUNT_KEY_LEN + sizeof(salt)];
	uint8_t h[FP_CRYPTO_SHA256_HASH_LEN];
	uint32_t x;
	uint32_t m;

	memset(out, 0, s);
	for (size_t i = 0; i < n; i++) {
		memcpy(v, account_key_list[i].key, FP_ACCOUNT_KEY_LEN);
		v[FP_ACCOUNT_KEY_LEN] = salt;
		int err = fp_crypto_sha256(h, v, sizeof(v));

		if (err) {
			return err;
		}
		for (size_t j = 0; j < FP_CRYPTO_SHA256_HASH_LEN / sizeof(x); j++) {
			x = sys_get_be32(&h[j * sizeof(x)]);
			m = x % (s * __CHAR_BIT__);
			WRITE_BIT(out[m / __CHAR_BIT__], m % __CHAR_BIT__, 1);
		}
	}
	return 0;
}

int fp_crypto_additional_data_encode(uint8_t *out_packet, const uint8_t *data, size_t data_len,
				     const uint8_t *aes_key, const uint8_t *nonce)
{
	int err;
	uint8_t hmac_sha256_buf[FP_CRYPTO_SHA256_HASH_LEN];

	memcpy(&out_packet[ADDITIONAL_DATA_NONCE_POS], nonce, FP_CRYPTO_ADDITIONAL_DATA_NONCE_LEN);

	err = fp_crypto_aes128_ctr_encrypt(&out_packet[ADDITIONAL_DATA_ENC_DATA_POS], data,
					   data_len, aes_key, nonce);
	if (err) {
		return err;
	}

	err = fp_crypto_hmac_sha256(hmac_sha256_buf, &out_packet[ADDITIONAL_DATA_NONCE_POS],
				    FP_CRYPTO_ADDITIONAL_DATA_NONCE_LEN + data_len, aes_key);
	if (err) {
		return err;
	}

	memcpy(&out_packet[ADDITIONAL_DATA_SHA_POS], hmac_sha256_buf, ADDITIONAL_DATA_SHA_LEN);

	return 0;
}

int fp_crypto_additional_data_decode(uint8_t *out_data, const uint8_t *in_packet, size_t packet_len,
				     const uint8_t *aes_key)
{
	int err;
	uint8_t hmac_sha256_buf[FP_CRYPTO_SHA256_HASH_LEN];
	int decoded_data_len;

	if (packet_len > INT_MAX) {
		return -ENOMEM;
	}

	decoded_data_len = packet_len - FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN;
	if (decoded_data_len < 0) {
		return -EINVAL;
	}

	err = fp_crypto_hmac_sha256(hmac_sha256_buf, &in_packet[ADDITIONAL_DATA_NONCE_POS],
				    packet_len - ADDITIONAL_DATA_SHA_LEN, aes_key);
	if (err) {
		return err;
	}
	/* Check packet integrity. */
	if (memcmp(&in_packet[ADDITIONAL_DATA_SHA_POS], hmac_sha256_buf, ADDITIONAL_DATA_SHA_LEN)) {
		return -EINVAL;
	}

	return fp_crypto_aes128_ctr_decrypt(out_data, &in_packet[ADDITIONAL_DATA_ENC_DATA_POS],
					    decoded_data_len, aes_key,
					    &in_packet[ADDITIONAL_DATA_NONCE_POS]);
}
