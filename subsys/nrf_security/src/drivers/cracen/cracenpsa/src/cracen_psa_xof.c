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
#include <cracen/statuscodes.h>
#include <zephyr/sys/__assert.h>
#include <cracen/common.h>
#include <nrf_security_mem_helpers.h>
#include "cracen_psa_primitives.h"

psa_status_t cracen_xof_setup(cracen_xof_operation_t *operation, psa_algorithm_t alg)
{
	psa_status_t status;

	status = xof_get_algo(alg, &operation->hash_op.sx_hash_algo);
	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->hash_op.has_saved_state = false;
	operation->prev_squeezed = 0;
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
	int sx_status;
	size_t block_sz;

	if (output_length == 0) {
		return PSA_SUCCESS;
	}

	__ASSERT_NO_MSG(output != NULL);

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
					 output,
					 output_length);
	if (sx_status != SX_OK) {
		goto exit;
	}

	sx_status = sx_hash_wait(&operation->hash_op.sx_ctx);
	if (sx_status == SX_OK) {
		operation->prev_squeezed += output_length;
	}

exit:
	sx_hw_release(&operation->hash_op.sx_ctx.dma);
	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_xof_abort(cracen_xof_operation_t *operation)
{
	safe_memzero(operation, sizeof(*operation));
	return PSA_SUCCESS;
}
