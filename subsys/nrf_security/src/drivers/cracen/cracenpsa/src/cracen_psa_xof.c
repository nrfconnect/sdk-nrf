/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa_xof.h>
#include <cracen_psa_hash.h>

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <string.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/internal.h>
#include <sxsymcrypt/hashdefs.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <cracen/common.h>
#include "cracen_psa_primitives.h"

_Static_assert(SX_XOF_POOL_BUF_SZ != 1,
	       "To compile this file you need at least one XOF algorithm enabled in the driver "
	       "using the PSA_WANT_* configs.");

static psa_status_t get_possible_squeeze_size(cracen_xof_operation_t *operation,
					      size_t req_squeeze_sz,
					      size_t *possible_squeeze_sz)
{
	/** HW has a limitation on maximum possible output.
	 *  This lets callers reach SX_HASH_DIGESTSZ_SHAKE_MAX and keeps
	 *  the final refill from failing with SX_ERR_TOO_BIG.
	 */
	*possible_squeeze_sz = MIN(req_squeeze_sz,
				   (size_t)SX_HASH_DIGESTSZ_SHAKE_MAX - operation->prev_squeezed);
	if (*possible_squeeze_sz == 0) {
		/* Output budget exhausted. */
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return PSA_SUCCESS;
}

/**
 * @brief Squeeze bytes of XOF output into a buffer.
 *
 *	  The BA418 does not support output continuation, so the stream is
 *	  regenerated from scratch on every call and the already-emitted
 *	  prefix (operation->prev_squeezed bytes) is discarded, increasing
 *	  the cost of each HW call.
 *
 * @param[in, out] operation  XOF operation context
 * @param[out]     out        Squeezed output bytes
 * @param[in]      squeeze_sz Number of bytes to squeeze
 *
 * @return psa_status_t
 */
static psa_status_t xof_squeeze(cracen_xof_operation_t *operation, uint8_t *out,
				size_t squeeze_sz)
{
	int sx_status;
	size_t block_sz;

	sx_status = sx_hw_reserve(&operation->hash_op.sx_ctx.dma, SX_HW_RESERVE_DEFAULT);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	if (operation->hash_op.has_saved_state) {
		sx_status = sx_hash_resume_state(&operation->hash_op.sx_ctx);
	} else {
		sx_status = sx_hash_create(&operation->hash_op.sx_ctx,
					   operation->hash_op.sx_hash_algo,
					   sizeof(operation->hash_op.sx_ctx));
	}

	if (sx_status != SX_OK) {
		goto exit;
	}

	block_sz = sx_hash_get_alg_blocksz(operation->hash_op.sx_hash_algo);
	sx_status = sx_hash_feed(&operation->hash_op.sx_ctx, operation->hash_op.input_buffer,
				 block_sz - operation->hash_op.bytes_left_for_next_block);
	if (sx_status != SX_OK) {
		goto exit;
	}

	sx_status = sx_hash_shake_digest(&operation->hash_op.sx_ctx,
					 operation->prev_squeezed,
					 out,
					 squeeze_sz);
	if (sx_status != SX_OK) {
		goto exit;
	}

	sx_status = sx_hash_wait(&operation->hash_op.sx_ctx);

exit:
	sx_hw_release(&operation->hash_op.sx_ctx.dma);
	return silex_statuscodes_to_psa(sx_status);
}

/**
 * @brief Refill pool buffer with SX_XOF_POOL_BUF_SZ bytes squeezed from XOF function.
 *
 * @param[in, out] operation XOF operation context
 *
 * @return psa_status_t
 */
static psa_status_t xof_refill_pool(cracen_xof_operation_t *operation)
{
	psa_status_t status;
	size_t squeeze_sz;

	status = get_possible_squeeze_size(operation, SX_XOF_POOL_BUF_SZ, &squeeze_sz);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = xof_squeeze(operation, operation->pool, squeeze_sz);
	if (status == PSA_SUCCESS) {
		operation->pool_offset = 0;
		operation->pool_avail = squeeze_sz;
	}

	return status;
}

psa_status_t cracen_xof_setup(cracen_xof_operation_t *operation, psa_algorithm_t alg)
{
	psa_status_t status;

	status = cracen_xof_get_algo(alg, &operation->hash_op.sx_hash_algo);
	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->hash_op.has_saved_state = false;
	operation->prev_squeezed = 0;
	operation->pool_offset = 0;
	operation->pool_avail = 0;
	operation->hash_op.bytes_left_for_next_block =
		sx_hash_get_alg_blocksz(operation->hash_op.sx_hash_algo);

	return PSA_SUCCESS;
}

psa_status_t cracen_xof_set_context(cracen_xof_operation_t *operation, const uint8_t *context,
				    size_t context_length)
{
	(void)operation;
	(void)context;
	(void)context_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_xof_update(cracen_xof_operation_t *operation, const uint8_t *input,
			       size_t input_length)
{
	return cracen_hash_update(&operation->hash_op, input, input_length);
}

psa_status_t cracen_xof_output(cracen_xof_operation_t *operation, uint8_t *output,
			       size_t output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t bytes_to_copy;

	if (output_length == 0) {
		return PSA_SUCCESS;
	}

	__ASSERT_NO_MSG(output != NULL);

	while (output_length > 0) {
		/** Optimization path: for large requests squeezing just into the caller's buffer,
		 *  bypassing pool, this allows to skip unnecessary memcpy operation.
		 *  For small requests keep going through the pool to decrease the time cost
		 *  of accessing HW for small chunks of data.
		 */
		if (operation->pool_avail == 0 &&
		    output_length >= sx_hash_get_alg_blocksz(operation->hash_op.sx_hash_algo)) {

			status = get_possible_squeeze_size(operation, output_length,
							   &bytes_to_copy);
			if (status != PSA_SUCCESS) {
				return status;
			}

			status = xof_squeeze(operation, output, bytes_to_copy);
			if (status != PSA_SUCCESS) {
				return status;
			}
		} else {
			if (operation->pool_avail == 0) {
				status = xof_refill_pool(operation);
				if (status != PSA_SUCCESS) {
					return status;
				}
			}

			bytes_to_copy = MIN(output_length, operation->pool_avail);
			memcpy(output, operation->pool + operation->pool_offset, bytes_to_copy);

			operation->pool_offset += bytes_to_copy;
			operation->pool_avail -= bytes_to_copy;
		}

		operation->prev_squeezed += bytes_to_copy;
		output += bytes_to_copy;
		output_length -= bytes_to_copy;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_xof_abort(cracen_xof_operation_t *operation)
{
	safe_memzero(operation, sizeof(*operation));
	return PSA_SUCCESS;
}
