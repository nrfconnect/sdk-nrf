/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <string.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/hashdefs.h>
#include <sxsymcrypt/keyref.h>
#include "common.h"
#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include "cracen_psa_primitives.h"
#include "hmac.h"

psa_status_t cracen_hmac_setup(cracen_mac_operation_t *operation,
				      const psa_key_attributes_t *attributes,
				      const uint8_t *key_buffer, size_t key_buffer_size,
				      psa_algorithm_t alg)
{
	int sx_status;
	const struct sxhashalg *sx_hash_algo = NULL;

	psa_status_t psa_status = hash_get_algo(PSA_ALG_HMAC_GET_HASH(alg), &sx_hash_algo);

	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	/* HMAC operation creation and configuration. */
	sx_status =  mac_create_hmac(sx_hash_algo, &operation->hmac.hashctx, key_buffer,
		key_buffer_size, operation->hmac.workmem, sizeof(operation->hmac.workmem));
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	operation->bytes_left_for_next_block = sx_hash_get_alg_blocksz(sx_hash_algo);
	operation->is_first_block = true;

	return PSA_SUCCESS;
}

psa_status_t cracen_hmac_update(cracen_mac_operation_t *operation, const uint8_t *input,
				       size_t input_length)
{
	int sx_status;
	size_t block_size;
	size_t input_chunk_length = 0;
	size_t remaining_bytes = 0;
	const struct sxhashalg *sx_hash_algo = NULL;

	/* As the block size is needed several times we compute it once here */
	psa_status_t psa_status = hash_get_algo(PSA_ALG_GET_HASH(operation->alg), &sx_hash_algo);

	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}
	block_size = sx_hash_get_alg_blocksz(sx_hash_algo);

	if (input_length < operation->bytes_left_for_next_block) {
		/* sx_hash_feed doesn't buffer the input data until
		 * sx_hash_save_state is called so a local buffer is used
		 */
		size_t offset = block_size - operation->bytes_left_for_next_block;

		memcpy(operation->input_buffer + offset, input, input_length);
		operation->bytes_left_for_next_block -= input_length;

		/* can't fill a full block so nothing more to do here */
		return PSA_SUCCESS;
	}

	if (!operation->is_first_block) {
		sx_status = sx_hash_resume_state(&operation->hmac.hashctx);
		if (sx_status != SX_OK) {
			return silex_statuscodes_to_psa(sx_status);
		}
	}

	operation->is_first_block = false;

	/* Feed the data that are currently in the input buffer to the driver */
	sx_status = sx_hash_feed(&operation->hmac.hashctx, operation->input_buffer,
			(block_size - operation->bytes_left_for_next_block));
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* Add fill up the next block and add as many full blocks as possible by
	 * adding as many input bytes as needed to get to be block aligned
	 */
	input_chunk_length = operation->bytes_left_for_next_block;
	input_chunk_length += ROUND_DOWN((input_length - input_chunk_length), block_size);
	remaining_bytes = input_length - input_chunk_length;

	/* forward the data to the driver */
	sx_status = sx_hash_feed(&operation->hmac.hashctx, input, input_chunk_length);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_hash_save_state(&operation->hmac.hashctx);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_hash_wait(&operation->hmac.hashctx);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* Input buffer was processed, reset the bytes for the next block and
	 * empty input buffer
	 */
	operation->bytes_left_for_next_block = block_size;
	safe_memzero(operation->input_buffer, sizeof(operation->input_buffer));

	/* Copy the remaining bytes to the local input buffer */
	if (remaining_bytes > 0) {
		memcpy(operation->input_buffer, input + input_chunk_length, remaining_bytes);
		operation->bytes_left_for_next_block -= remaining_bytes;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_hmac_finish(cracen_mac_operation_t *operation)
{
	int sx_status;
	size_t block_size, digestsz;
	const struct sxhashalg *sx_hash_algo = NULL;

	sx_status = hash_get_algo(PSA_ALG_GET_HASH(operation->alg), &sx_hash_algo);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	digestsz = sx_hash_get_alg_digestsz(sx_hash_algo);
	block_size = sx_hash_get_alg_blocksz(sx_hash_algo);

	if (!operation->is_first_block) {
		sx_status = sx_hash_resume_state(&operation->hmac.hashctx);
		if (sx_status != SX_OK) {
			return silex_statuscodes_to_psa(sx_status);
		}
	}

	/* Process the data that are left in the input buffer. */
	sx_status = sx_hash_feed(&operation->hmac.hashctx, operation->input_buffer,
			block_size - operation->bytes_left_for_next_block);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* Generate the MAC. */
	sx_status = hmac_produce(&operation->hmac.hashctx, sx_hash_algo, operation->input_buffer,
			      sizeof(operation->input_buffer), operation->hmac.workmem);

	return silex_statuscodes_to_psa(sx_status);
}
