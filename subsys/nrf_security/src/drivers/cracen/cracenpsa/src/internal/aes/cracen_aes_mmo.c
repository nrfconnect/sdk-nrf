/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* AES-MMO as specified by the Zigbee specification, clause B.6: a Merkle-Damgard
 * construction over a Matyas-Meyer-Oseas one-way compression function that uses
 * AES-128 as the block cipher:
 *
 *     H_0 = 0, H_i = AES(key = H_(i-1), M_i) XOR M_i
 *
 * CRACEN has no hardware mode for this. The AES key is the running digest, so it
 * changes for every message block and the hardware cannot chain the blocks by
 * itself (BA411E AES datasheet, clause 6.3: the key can only be reprogrammed once
 * the previous packet has been fully processed). Every block is therefore a
 * separate single-block AES-ECB operation on the hardware, with the chaining and
 * the XOR done here.
 */

#include <internal/aes/cracen_aes_mmo.h>

#include <cracen/common.h>
#include <cracen/statuscodes.h>
#include <nrf_security_mem_helpers.h>
#include <string.h>
#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/internal.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

/* Zigbee clause B.6 encodes the message length as a 16-bit bit count in the last
 * two octets of the final block, which limits the message to 2^16 - 1 bits.
 */
#define AES_MMO_MAX_MESSAGE_LENGTH (UINT16_MAX / 8)

/* CRACEN writes the AES output with DMA and invalidates the cache lines that the
 * output buffer occupies, which would discard unrelated dirty data sharing those
 * lines. Keep the output in its own cache-line aligned object. It is only used
 * while the CRACEN symmetric mutex is held, which is taken by sx_hw_reserve();
 * this is the same reasoning that lets sx_blkcipher_ecb_simple() keep its DMA
 * descriptors static.
 */
#if defined(CONFIG_DCACHE_LINE_SIZE) && (CONFIG_DCACHE_LINE_SIZE > 0)
#define AES_MMO_DMA_ALIGNMENT CONFIG_DCACHE_LINE_SIZE
#else
#define AES_MMO_DMA_ALIGNMENT sizeof(uint32_t)
#endif

static uint8_t aes_mmo_ciphertext[ROUND_UP(CRACEN_AES_MMO_BLOCK_SIZE, AES_MMO_DMA_ALIGNMENT)]
	__aligned(AES_MMO_DMA_ALIGNMENT);

/* Run one Matyas-Meyer-Oseas compression round on the block held by the operation.
 * The CRACEN hardware must be reserved by the caller.
 */
static psa_status_t cracen_aes_mmo_compress(cracen_aes_mmo_operation_t *operation)
{
	int sx_status;

	sx_status = sx_blkcipher_ecb_simple(operation->digest, sizeof(operation->digest),
					    operation->block, CRACEN_AES_MMO_BLOCK_SIZE,
					    aes_mmo_ciphertext, CRACEN_AES_MMO_BLOCK_SIZE);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	for (size_t i = 0; i < CRACEN_AES_MMO_BLOCK_SIZE; i++) {
		operation->digest[i] = aes_mmo_ciphertext[i] ^ operation->block[i];
	}

	safe_memzero(aes_mmo_ciphertext, sizeof(aes_mmo_ciphertext));

	return PSA_SUCCESS;
}

psa_status_t cracen_aes_mmo_setup(cracen_aes_mmo_operation_t *operation)
{
	__ASSERT_NO_MSG(operation != NULL);

	memset(operation, 0, sizeof(*operation));

	return PSA_SUCCESS;
}

psa_status_t cracen_aes_mmo_update(cracen_aes_mmo_operation_t *operation, const uint8_t *input,
				   size_t input_length)
{
	psa_status_t status = PSA_SUCCESS;
	int sx_status;

	__ASSERT_NO_MSG(operation != NULL);

	/* Valid PSA call, just nothing to do */
	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	__ASSERT_NO_MSG(input != NULL);

	if (input_length > AES_MMO_MAX_MESSAGE_LENGTH - operation->message_length) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	operation->message_length += input_length;

	/* Only reserve the hardware if this fragment completes at least one block.
	 * The remaining bytes are buffered until a later update, or until finish()
	 * pads them.
	 */
	if (operation->block_length + input_length < CRACEN_AES_MMO_BLOCK_SIZE) {
		memcpy(operation->block + operation->block_length, input, input_length);
		operation->block_length += input_length;
		return PSA_SUCCESS;
	}

	sx_status = sx_hw_reserve(NULL, SX_HW_RESERVE_DEFAULT);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	while (input_length > 0) {
		size_t chunk_length =
			MIN(CRACEN_AES_MMO_BLOCK_SIZE - operation->block_length, input_length);

		memcpy(operation->block + operation->block_length, input, chunk_length);
		operation->block_length += chunk_length;
		input += chunk_length;
		input_length -= chunk_length;

		if (operation->block_length == CRACEN_AES_MMO_BLOCK_SIZE) {
			status = cracen_aes_mmo_compress(operation);
			if (status != PSA_SUCCESS) {
				break;
			}
			operation->block_length = 0;
		}
	}

	sx_hw_release(NULL);

	return status;
}

psa_status_t cracen_aes_mmo_finish(cracen_aes_mmo_operation_t *operation, uint8_t *hash,
				   size_t hash_size, size_t *hash_length)
{
	psa_status_t status;
	int sx_status;
	size_t message_bits;

	__ASSERT_NO_MSG(operation != NULL);
	__ASSERT_NO_MSG(hash_length != NULL);

	if (hash_size < CRACEN_AES_MMO_BLOCK_SIZE) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	*hash_length = 0;
	message_bits = operation->message_length * 8;

	sx_status = sx_hw_reserve(NULL, SX_HW_RESERVE_DEFAULT);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* Zigbee clause B.6 padding: a single 1 bit, then zero bits, then the message
	 * bit length in the last two octets of the final block. If the 1 bit does not
	 * leave room for the length field, the length goes into an additional block.
	 */
	memset(operation->block + operation->block_length, 0,
	       CRACEN_AES_MMO_BLOCK_SIZE - operation->block_length);
	operation->block[operation->block_length] = 0x80;

	if (operation->block_length >= CRACEN_AES_MMO_BLOCK_SIZE - 2) {
		status = cracen_aes_mmo_compress(operation);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
		memset(operation->block, 0, CRACEN_AES_MMO_BLOCK_SIZE);
	}

	operation->block[CRACEN_AES_MMO_BLOCK_SIZE - 2] = (uint8_t)(message_bits >> 8);
	operation->block[CRACEN_AES_MMO_BLOCK_SIZE - 1] = (uint8_t)message_bits;

	status = cracen_aes_mmo_compress(operation);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	memcpy(hash, operation->digest, CRACEN_AES_MMO_BLOCK_SIZE);
	*hash_length = CRACEN_AES_MMO_BLOCK_SIZE;

exit:
	sx_hw_release(NULL);

	return status;
}
