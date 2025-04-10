/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <sicrypto/hmac.h>
#include <sicrypto/sicrypto.h>
#include <string.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/hashdefs.h>
#include <sxsymcrypt/keyref.h>
#include "common.h"
#include <cracen/mem_helpers.h>
#include "cracen_psa_primitives.h"

psa_status_t cracen_hmac_setup(cracen_mac_operation_t *operation,
				      const psa_key_attributes_t *attributes,
				      const uint8_t *key_buffer, size_t key_buffer_size,
				      psa_algorithm_t alg)
{
	int status;
	const struct sxhashalg *sx_hash_algo = NULL;

	psa_status_t psa_status = hash_get_algo(PSA_ALG_HMAC_GET_HASH(alg), &sx_hash_algo);

	if (psa_status) {
		return psa_status;
	}

	/* HMAC task creation and configuration. */
	si_task_init(&operation->hmac.task, operation->hmac.workmem,
		     SX_HASH_MAX_ENABLED_BLOCK_SIZE + PSA_HASH_MAX_SIZE);
	si_mac_create_hmac(&operation->hmac.task, sx_hash_algo, key_buffer, key_buffer_size);

	/* Wait until the key is processed */
	si_task_partial_run(&operation->hmac.task);
	status = si_task_wait(&operation->hmac.task);
	if (status != SX_ERR_READY) {
		return silex_statuscodes_to_psa(status);
	}

	operation->bytes_left_for_next_block = sx_hash_get_alg_blocksz(sx_hash_algo);

	return PSA_SUCCESS;
}

psa_status_t cracen_hmac_update(cracen_mac_operation_t *operation, const uint8_t *input,
				       size_t input_length)
{
	int status;
	size_t block_size;
	size_t input_chunk_length = 0;
	size_t remaining_bytes = 0;
	const struct sxhashalg *sx_hash_algo = NULL;

	/* As the block size is needed several times we compute it once here */
	psa_status_t psa_status = hash_get_algo(PSA_ALG_GET_HASH(operation->alg), &sx_hash_algo);

	if (psa_status) {
		return psa_status;
	}
	block_size = sx_hash_get_alg_blocksz(sx_hash_algo);

	if (input_length < operation->bytes_left_for_next_block) {
		/* si_task_consume doesn't buffer the input data until
		 * si_task_partial_run is called so a local buffer is used
		 */
		size_t offset = block_size - operation->bytes_left_for_next_block;

		memcpy(operation->input_buffer + offset, input, input_length);
		operation->bytes_left_for_next_block -= input_length;

		/* can't fill a full block so nothing more to do here */
		return PSA_SUCCESS;
	}

	/* Feed the data that are currently in the input buffer to the driver */
	si_task_consume(&operation->hmac.task, operation->input_buffer,
			(block_size - operation->bytes_left_for_next_block));

	/* Add fill up the next block and add as many full blocks as possible by
	 * adding as many input bytes as needed to get to be block aligned
	 */
	input_chunk_length = operation->bytes_left_for_next_block;
	input_chunk_length += ROUND_DOWN((input_length - input_chunk_length), block_size);
	remaining_bytes = input_length - input_chunk_length;

	/* forward the data to the driver */
	si_task_consume(&operation->hmac.task, input, input_chunk_length);

	/* start partial run and wait until processed */
	si_task_partial_run(&operation->hmac.task);
	status = si_task_wait(&operation->hmac.task);
	if (status != SX_ERR_READY) {
		return silex_statuscodes_to_psa(status);
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
	int status;
	size_t block_size, digestsz;
	const struct sxhashalg *sx_hash_algo = NULL;

	status = hash_get_algo(PSA_ALG_GET_HASH(operation->alg), &sx_hash_algo);
	if (status != SX_OK) {
		return silex_statuscodes_to_psa(status);
	}

	digestsz = sx_hash_get_alg_digestsz(sx_hash_algo);
	block_size = sx_hash_get_alg_blocksz(sx_hash_algo);

	/* Process the data that are left in the input buffer, also a call to */
	/* si_task_consume is anyway needed before calling si_task_produce */
	si_task_consume(&operation->hmac.task, operation->input_buffer,
			block_size - operation->bytes_left_for_next_block);

	/* Generate the MAC. */
	si_task_produce(&operation->hmac.task, operation->input_buffer, digestsz);

	/* Wait until the task completes. */
	status = si_task_wait(&operation->hmac.task);
	if (status != SX_OK) {
		return silex_statuscodes_to_psa(status);
	}

	return PSA_SUCCESS;
}
