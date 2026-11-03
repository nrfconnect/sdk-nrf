/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdint.h>
#include <string.h>
#include <psa/crypto.h>
#include <cracen/statuscodes.h>
#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/internal.h>
#include <sxsymcrypt/keyref.h>
#include <sxsymcrypt/aes.h>
#include <cracen/common.h>
#include <cracen_sw_common.h>
#include <cracen_sw_mac_cmac.h>

#define CMAC_PADDING_BYTE (0x80)
#define AES_CMAC_MSB	  (0x80)
#define CMAC_CONSTANT_RB  (0x87)

psa_status_t cracen_sw_cmac_setup(cracen_mac_operation_t *operation,
				  const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				  size_t key_buffer_size)
{
	psa_status_t status = PSA_SUCCESS;
	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));

	/* Only AES-CMAC is supported */
	if (psa_get_key_type(attributes) != PSA_KEY_TYPE_AES) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (key_buffer_size != 16 && key_buffer_size != 24 && key_buffer_size != 32 &&
	    location != PSA_KEY_LOCATION_CRACEN_KMU && location != PSA_KEY_LOCATION_CRACEN) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	/* Load the key reference */
	if (key_buffer_size > sizeof(operation->cmac.key_buffer)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	memcpy(operation->cmac.key_buffer, key_buffer, key_buffer_size);
	status = cracen_load_keyref(attributes, operation->cmac.key_buffer, key_buffer_size,
				    &operation->cmac.keyref);
	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->bytes_left_for_next_block = SX_BLKCIPHER_AES_BLK_SZ;
	operation->has_saved_state = false;

	return PSA_SUCCESS;
}

static void left_shift_block(const uint8_t *in, uint8_t *out)
{
	uint8_t carry = 0;
	uint8_t byte;

	for (int i = SX_BLKCIPHER_AES_BLK_SZ - 1; i >= 0; i--) {
		byte = in[i];
		out[i] = (byte << 1) | carry;
		carry = (byte & AES_CMAC_MSB) ? 1 : 0;
	}
}

psa_status_t cracen_cmac_derive_subkeys(cracen_mac_operation_t *operation, uint8_t *k1, uint8_t *k2)
{
	uint8_t empty_block[SX_BLKCIPHER_AES_BLK_SZ] = {0};
	uint8_t L[SX_BLKCIPHER_AES_BLK_SZ]; /* L is defined in RFC 4493 */
	psa_status_t status = cracen_sw_aes_primitive(&operation->cmac.cipher,
						      &operation->cmac.keyref,
						      empty_block, L);

	if (status != PSA_SUCCESS) {
		return status;
	}

	left_shift_block(L, k1);
	if (L[0] & AES_CMAC_MSB) {
		k1[SX_BLKCIPHER_AES_BLK_SZ - 1] ^= CMAC_CONSTANT_RB;
	}

	left_shift_block(k1, k2);
	if (k1[0] & AES_CMAC_MSB) {
		k2[SX_BLKCIPHER_AES_BLK_SZ - 1] ^= CMAC_CONSTANT_RB;
	}
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_cmac_update(cracen_mac_operation_t *operation, const uint8_t *data,
				   size_t data_len)
{
	cracen_cmac_context_t *sw_ctx = &operation->cmac.sw_ctx;
	psa_status_t psa_status = PSA_ERROR_CORRUPTION_DETECTED;

	/* A full block is only encrypted once more data follows it, as finish() treats
	 * the last block apart. Until then nothing needs CRACEN.
	 */
	if (data_len <= SX_BLKCIPHER_AES_BLK_SZ - sw_ctx->partial_len) {
		memcpy(&sw_ctx->partial_block[sw_ctx->partial_len], data, data_len);
		sw_ctx->partial_len += data_len;
		return PSA_SUCCESS;
	}

	/* CRACEN stays reserved for all blocks of this call */
	psa_status = silex_statuscodes_to_psa(
		sx_hw_reserve(&operation->cmac.cipher.dma, SX_HW_RESERVE_CM_ENABLED));
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	psa_status = cracen_sw_cbc_mac_update(&operation->cmac.cipher, &operation->cmac.keyref,
					      sw_ctx->mac_state, sw_ctx->partial_block,
					      &sw_ctx->partial_len, data, data_len, true);

	sx_hw_release(&operation->cmac.cipher.dma);
	return psa_status;
}

psa_status_t cracen_sw_cmac_finish(cracen_mac_operation_t *operation)
{
	psa_status_t psa_status;
	uint8_t last_block[SX_BLKCIPHER_AES_BLK_SZ] = {0};
	uint8_t k1[SX_BLKCIPHER_AES_BLK_SZ]; /* k1 is defined in RFC 4493 */
	uint8_t k2[SX_BLKCIPHER_AES_BLK_SZ]; /* k2 is defined in RFC 4493 */

	if (operation->mac_size > SX_BLKCIPHER_AES_BLK_SZ) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* CRACEN stays reserved for all blocks of this call */
	psa_status = silex_statuscodes_to_psa(
		sx_hw_reserve(&operation->cmac.cipher.dma, SX_HW_RESERVE_CM_ENABLED));
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	psa_status = cracen_cmac_derive_subkeys(operation, k1, k2);
	if (psa_status != PSA_SUCCESS) {
		goto exit;
	}

	/*  If full block, XOR with K1; otherwise pad and XOR with K2 */
	if (operation->cmac.sw_ctx.partial_len == SX_BLKCIPHER_AES_BLK_SZ) {
		cracen_xorbytes(operation->cmac.sw_ctx.partial_block, k1, SX_BLKCIPHER_AES_BLK_SZ);
		memcpy(last_block, operation->cmac.sw_ctx.partial_block, SX_BLKCIPHER_AES_BLK_SZ);
	} else {
		memcpy(last_block, operation->cmac.sw_ctx.partial_block,
		       operation->cmac.sw_ctx.partial_len);

		last_block[operation->cmac.sw_ctx.partial_len] = CMAC_PADDING_BYTE;
		cracen_xorbytes(last_block, k2, SX_BLKCIPHER_AES_BLK_SZ);
	}

	/* Final MAC: encrypt (mac_state XOR last_block) */
	cracen_xorbytes(operation->cmac.sw_ctx.mac_state, last_block, SX_BLKCIPHER_AES_BLK_SZ);

	psa_status = cracen_sw_aes_primitive(&operation->cmac.cipher, &operation->cmac.keyref,
					     operation->cmac.sw_ctx.mac_state,
					     operation->cmac.sw_ctx.mac_state);
	if (psa_status != PSA_SUCCESS) {
		goto exit;
	}

	memcpy(operation->input_buffer, operation->cmac.sw_ctx.mac_state, operation->mac_size);

	psa_status = PSA_SUCCESS;
exit:
	sx_hw_release(&operation->cmac.cipher.dma);
	return psa_status;
}

/* Single-shot operations still use hw cmac */
psa_status_t cracen_cmac_compute(cracen_mac_operation_t *operation, const uint8_t *input,
				 size_t input_length)
{
	int sx_status;

	sx_status = sx_hw_reserve(&operation->cmac.ctx.dma, SX_HW_RESERVE_CM_ENABLED);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_mac_create_aescmac(&operation->cmac.ctx, &operation->cmac.keyref);
	if (sx_status != SX_OK) {
		goto exit;
	}

	sx_status = sx_mac_feed(&operation->cmac.ctx, input, input_length);
	if (sx_status != SX_OK) {
		goto exit;
	}

	/* Writes a full CMAC block, so it goes to the internal buffer. Same as the non-workaround
	 * driver does in cracen_mac_cmac.c.
	 */
	sx_status = sx_mac_generate(&operation->cmac.ctx, operation->input_buffer);
	if (sx_status != SX_OK) {
		goto exit;
	}

	sx_status = sx_mac_wait(&operation->cmac.ctx);

exit:
	sx_hw_release(&operation->cmac.ctx.dma);
	return silex_statuscodes_to_psa(sx_status);
}
