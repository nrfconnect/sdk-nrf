/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/key_wrap/cracen_key_wrap_kw.h>
#include <internal/key_wrap/cracen_key_wrap_common.h>

#include <stddef.h>
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <silexpk/core.h>

/* RFC3394 */
#define CRACEN_KW_MIN_KEY_SIZE	(2 * CRACEN_KEY_WRAP_BLOCK_SIZE)

/* AES-KW default initial value (RFC3394) */
static const uint8_t cracen_kw_iv[CRACEN_KEY_WRAP_BLOCK_SIZE] = {0xA6, 0xA6, 0xA6, 0xA6,
								 0xA6, 0xA6, 0xA6, 0xA6};

psa_status_t cracen_key_wrap_kw_wrap(const psa_key_attributes_t *wrapping_key_attributes,
				     const uint8_t *wrapping_key_data, size_t wrapping_key_size,
				     const uint8_t *key_data, size_t key_size,
				     uint8_t *data, size_t data_size, size_t *data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t blocks_count;
	struct sxkeyref keyref;
	psa_key_type_t key_type = psa_get_key_type(wrapping_key_attributes);

	if (key_type != PSA_KEY_TYPE_AES) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (key_size < CRACEN_KW_MIN_KEY_SIZE || key_size % CRACEN_KEY_WRAP_BLOCK_SIZE != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* Ciphertext contains (n+1) AES KW blocks since it adds integrity check register */
	if (key_size + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE > data_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* "data" contains both integrity check register and wrapped key */
	memcpy(data, cracen_kw_iv, CRACEN_KEY_WRAP_BLOCK_SIZE);
	memmove(data + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE, key_data, key_size);
	blocks_count = key_size / CRACEN_KEY_WRAP_BLOCK_SIZE;

	status = cracen_load_keyref(wrapping_key_attributes, wrapping_key_data, wrapping_key_size,
				    &keyref);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_key_wrap(&keyref, data + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE,
				 blocks_count, data);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	*data_length = key_size + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE;
	return status;
exit:
	safe_memzero(data, data_size);
	return status;
}

psa_status_t cracen_key_wrap_kw_unwrap(const psa_key_attributes_t *wrapping_key_attributes,
				       const uint8_t *wrapping_key_data, size_t wrapping_key_size,
				       const uint8_t *data, size_t data_size,
				       uint8_t *key_data, size_t key_data_size,
				       size_t *key_data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t blocks_count;
	struct sxkeyref keyref;
	uint8_t cipher_input_block[SX_BLKCIPHER_AES_BLK_SZ];
	int sig_cmp_res;
	psa_key_type_t key_type = psa_get_key_type(wrapping_key_attributes);

	if (key_type != PSA_KEY_TYPE_AES) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (data_size < (CRACEN_KW_MIN_KEY_SIZE + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE) ||
	    data_size % CRACEN_KEY_WRAP_BLOCK_SIZE != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_size - CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE > key_data_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memmove(key_data,
		data + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE,
		data_size - CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE);
	blocks_count =
		(data_size - CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE) / CRACEN_KEY_WRAP_BLOCK_SIZE;
	memcpy(cipher_input_block, data, CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE);

	status = cracen_load_keyref(wrapping_key_attributes, wrapping_key_data, wrapping_key_size,
				    &keyref);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_key_unwrap(&keyref, cipher_input_block, key_data, blocks_count);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	sig_cmp_res = constant_memcmp(cipher_input_block, cracen_kw_iv,
				      CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE);
	safe_memzero(cipher_input_block, SX_BLKCIPHER_AES_BLK_SZ);

	if (sig_cmp_res != 0) {
		status = PSA_ERROR_INVALID_SIGNATURE;
		goto exit;
	}

	*key_data_length = data_size - CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE;
	return status;
exit:
	safe_memzero(key_data, key_data_size);
	return status;
}
