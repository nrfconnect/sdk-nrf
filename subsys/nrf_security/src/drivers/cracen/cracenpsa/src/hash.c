/**
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <sicrypto/hash.h>
#include <string.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/internal.h>
#include <sxsymcrypt/sha1.h>
#include <sxsymcrypt/sha2.h>
#include <sxsymcrypt/sha3.h>
#include <cracen/statuscodes.h>
#include <zephyr/sys/__assert.h>
#include "common.h"
#include <cracen/mem_helpers.h>
#include "cracen_psa_primitives.h"

psa_status_t cracen_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				 uint8_t *hash, size_t hash_size, size_t *hash_length)
{
	int sx_status;
	struct sxhash c = {0};
	const struct sxhashalg *sx_hash_algo = NULL;

	psa_status_t psa_status = hash_get_algo(alg, &sx_hash_algo);

	if (psa_status) {
		return psa_status;
	}

	if (sx_hash_get_alg_digestsz(sx_hash_algo) > hash_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	*hash_length = sx_hash_get_alg_digestsz(sx_hash_algo);

	sx_status = sx_hash_create(&c, sx_hash_algo, sizeof(c));
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_hash_feed(&c, (char *)input, input_length);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_hash_digest(&c, (char *)hash);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_hash_wait(&c);

	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_hash_setup(cracen_hash_operation_t *operation, psa_algorithm_t alg)
{
	int status;

	status = hash_get_algo(alg, &operation->sx_hash_algo);
	if (status) {
		return status;
	}
	operation->is_first_block = true;

	operation->bytes_left_for_next_block = sx_hash_get_alg_blocksz(operation->sx_hash_algo);

	return silex_statuscodes_to_psa(status);
}

static int init_or_resume_context(cracen_hash_operation_t *operation)
{
	int sx_status;

	/* The SX driver expects to have gotten some data, before being able to
	 * save the context,
	 * therefore the create call is done here, then feed data and then safe
	 * the context.
	 */
	if (operation->is_first_block == true) {
		sx_status = sx_hash_create(&operation->sx_ctx, operation->sx_hash_algo,
					   sizeof(operation->sx_ctx));
		if (sx_status) {
			return sx_status;
		}
		operation->is_first_block = false;

	} else {
		/* Get back the old state if previous operation had been done */
		sx_status = sx_hash_resume_state(&operation->sx_ctx);
		if (sx_status) {
			return sx_status;
		}
	}

	return SX_OK;
}

psa_status_t cracen_hash_update(cracen_hash_operation_t *operation, const uint8_t *input,
				const size_t input_length)
{
	int sx_status;
	size_t input_chunk_length = 0;
	size_t remaining_bytes = 0;

	/* Valid PSA call, just nothing to do */
	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	/* After input_length check as it might be valid to have length 0 with
	 * input == NULL
	 */
	__ASSERT_NO_MSG(input != NULL);

	if (input_length < operation->bytes_left_for_next_block) {
		/* sx_hash_feed doesn't buffer the input data until sx_hash_wait
		 * is called so a local buffer is used.
		 */
		size_t offset = sx_hash_get_alg_blocksz(operation->sx_hash_algo) -
				operation->bytes_left_for_next_block;
		memcpy(operation->input_buffer + offset, input, input_length);
		operation->bytes_left_for_next_block -= input_length;

		/* can't fill a full block so nothing more to do here */
		return PSA_SUCCESS;
	}

	/* Initialize or resume an already initialized context. */
	sx_status = init_or_resume_context(operation);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* Feed the data that are currently in the input buffer to the driver. */
	sx_status = sx_hash_feed(&operation->sx_ctx, operation->input_buffer,
				 sx_hash_get_alg_blocksz(operation->sx_hash_algo) -
					 operation->bytes_left_for_next_block);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* Add as many full blocks as possible by adding as many input bytes as
	 * needed to get to be block aligned. The block size is not guaranteed to be
	 * power of two.
	 */
	input_chunk_length = input_length - ((input_length - operation->bytes_left_for_next_block) %
					     sx_hash_get_alg_blocksz(operation->sx_hash_algo));
	remaining_bytes = input_length - input_chunk_length;

	/* forward the data to the driver and process the data */
	sx_status = sx_hash_feed(&operation->sx_ctx, input, input_chunk_length);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}
	sx_status = sx_hash_save_state(&operation->sx_ctx);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* Wait until partial processing is done */
	sx_status = sx_hash_wait(&operation->sx_ctx);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* As we just passed the last block reset the remaining size and clean
	 * input buffer
	 */
	operation->bytes_left_for_next_block = sx_hash_get_alg_blocksz(operation->sx_hash_algo);
	safe_memzero(operation->input_buffer, sizeof(operation->input_buffer));

	/* Copy the remaining bytes to the local input buffer */
	if (remaining_bytes > 0) {
		memcpy(operation->input_buffer, input + input_chunk_length, remaining_bytes);
		operation->bytes_left_for_next_block -= remaining_bytes;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_hash_finish(cracen_hash_operation_t *operation, uint8_t *hash, size_t hash_size,
				size_t *hash_length)
{
	int sx_status;

	__ASSERT_NO_MSG(hash_length != NULL);

	if (sx_hash_get_alg_digestsz(operation->sx_hash_algo) > hash_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	sx_status = init_or_resume_context(operation);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_hash_feed(&operation->sx_ctx, operation->input_buffer,
				 sx_hash_get_alg_blocksz(operation->sx_hash_algo) -
					 operation->bytes_left_for_next_block);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_hash_digest(&operation->sx_ctx, hash);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}
	sx_status = sx_hash_wait(&operation->sx_ctx);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	*hash_length = sx_hash_get_alg_digestsz(operation->sx_hash_algo);

	return PSA_SUCCESS;
}
