#include <stdint.h>
#include <string.h>
#include <psa/crypto.h>
#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/keyref.h>
#include "cracen_lm20_mac_cmac.h"
#include <sxsymcrypt/aes.h>

#define AES_BLOCK_SIZE	  (16)
#define CMAC_PADDING_BYTE (0x80)
#define AES_CMAC_MSB	  (0x80)
#define CMAC_CONSTANT_RB  (0x87)



static psa_status_t cracen_aes_ecb_encrypt(const struct sxkeyref *key, const uint8_t *input,
				    uint8_t *output)
{
	struct sxblkcipher blkciph;
	int sx_status;

	sx_status = sx_blkcipher_create_aesecb_enc(&blkciph, key);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_crypt(&blkciph, input, AES_BLOCK_SIZE, output);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_run(&blkciph);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_wait(&blkciph);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_lm20_cmac_setup(cracen_mac_operation_t *operation,
				    const psa_key_attributes_t *attributes,
				    const uint8_t *key_buffer, size_t key_buffer_size)
{
	psa_status_t status = PSA_SUCCESS;
	int sx_status;

	/* Only AES-CMAC is supported */
	if (psa_get_key_type(attributes) != PSA_KEY_TYPE_AES) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (key_buffer_size != 16 && key_buffer_size != 32) {
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

	sx_status = sx_mac_create_aescmac(&operation->cmac.ctx, &operation->cmac.keyref);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	operation->bytes_left_for_next_block = AES_BLOCK_SIZE;
	operation->is_first_block = true;

	return PSA_SUCCESS;
}

static void left_shift_block(const uint8_t *in, uint8_t *out)
{
	uint8_t carry = 0;
	for (int i = AES_BLOCK_SIZE - 1; i >= 0; i--) {
		uint8_t byte = in[i];
		out[i] = (byte << 1) | carry;
		carry = (byte & AES_CMAC_MSB) ? 1 : 0;
	}
}

psa_status_t cracen_cmac_derive_subkeys(cracen_mac_operation_t *operation, uint8_t *k1, uint8_t *k2)
{
	uint8_t empty_block[AES_BLOCK_SIZE] = {0};
	uint8_t L[AES_BLOCK_SIZE]; /* L is defined in RFC 4493 */

	psa_status_t status = cracen_aes_ecb_encrypt(&operation->cmac.keyref, empty_block, L);
	if (status != PSA_SUCCESS) {
		return status;
	}

	left_shift_block(L, k1);
	if (L[0] & AES_CMAC_MSB) {
		k1[AES_BLOCK_SIZE - 1] ^= CMAC_CONSTANT_RB;
	}

	left_shift_block(k1, k2);
	if (k1[0] & AES_CMAC_MSB) {
		k2[AES_BLOCK_SIZE - 1] ^= CMAC_CONSTANT_RB;
	}
	return PSA_SUCCESS;
}

psa_status_t cracen_lm20_cmac_update(cracen_mac_operation_t *operation, const uint8_t *data,
				     size_t data_len)
{
	psa_status_t psa_status;
	size_t bytes_to_copy;
	size_t remaining = data_len;
	size_t offset = 0;

	while (remaining > 0) {
		bytes_to_copy = AES_BLOCK_SIZE - operation->cmac.sw_ctx.partial_len;
		if (bytes_to_copy > remaining) {
			bytes_to_copy = remaining;
		}

		memcpy(operation->cmac.sw_ctx.partial_block + operation->cmac.sw_ctx.partial_len,
		       data + offset, bytes_to_copy);
		operation->cmac.sw_ctx.partial_len += bytes_to_copy;
		offset += bytes_to_copy;
		remaining -= bytes_to_copy;

		/* Only encrypt a full block if we know more data is coming */
		if (operation->cmac.sw_ctx.partial_len == AES_BLOCK_SIZE && remaining > 0) {
			cracen_xorbytes(operation->cmac.sw_ctx.mac_state,
					operation->cmac.sw_ctx.partial_block, AES_BLOCK_SIZE);
			psa_status = cracen_aes_ecb_encrypt(&operation->cmac.keyref,
							    operation->cmac.sw_ctx.mac_state,
							    operation->cmac.sw_ctx.mac_state);
			if (psa_status != PSA_SUCCESS) {
				return psa_status;
			}
			operation->cmac.sw_ctx.partial_len = 0;
			memset(operation->cmac.sw_ctx.partial_block, 0, AES_BLOCK_SIZE);
		}
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_lm20_cmac_finish(cracen_mac_operation_t *operation)
{
	psa_status_t psa_status;
	uint8_t last_block[AES_BLOCK_SIZE] = {0};
	uint8_t k1[AES_BLOCK_SIZE]; /* k1 is defined in RFC 4493 */
	uint8_t k2[AES_BLOCK_SIZE]; /* k2 is defined in RFC 4493 */

	psa_status = cracen_cmac_derive_subkeys(operation, k1, k2);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	/*  If full block, XOR with K1; otherwise pad and XOR with K2 */
	if (operation->cmac.sw_ctx.partial_len == AES_BLOCK_SIZE) {
		cracen_xorbytes(operation->cmac.sw_ctx.partial_block, k1, AES_BLOCK_SIZE);
		memcpy(last_block, operation->cmac.sw_ctx.partial_block, AES_BLOCK_SIZE);
	} else {
		memcpy(last_block, operation->cmac.sw_ctx.partial_block,
		       operation->cmac.sw_ctx.partial_len);

		last_block[operation->cmac.sw_ctx.partial_len] = CMAC_PADDING_BYTE;
		cracen_xorbytes(last_block, k2, AES_BLOCK_SIZE);
	}

	/* Final MAC: encrypt (mac_state XOR last_block) */
	cracen_xorbytes(operation->cmac.sw_ctx.mac_state, last_block, AES_BLOCK_SIZE);

	psa_status =
		cracen_aes_ecb_encrypt(&operation->cmac.keyref, operation->cmac.sw_ctx.mac_state,
				       operation->cmac.sw_ctx.mac_state);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	memcpy(operation->input_buffer, operation->cmac.sw_ctx.mac_state, AES_BLOCK_SIZE);
	operation->mac_size = AES_BLOCK_SIZE;

	return PSA_SUCCESS;
}

/* Single-shot operations still use hw cmac */
psa_status_t cracen_lm20_cmac_compute(cracen_mac_operation_t *operation, const uint8_t *input,
				      size_t input_length, uint8_t *mac)
{
	int sx_status;

	sx_status = sx_mac_feed(&operation->cmac.ctx, input, input_length);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_mac_generate(&operation->cmac.ctx, mac);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_mac_wait(&operation->cmac.ctx);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return PSA_SUCCESS;
}