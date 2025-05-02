/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <string.h>
#include <sxsymcrypt/cmac.h>
#include <sxsymcrypt/keyref.h>
#include "common.h"
#include <cracen/mem_helpers.h>
#include "cracen_psa_primitives.h"

#define AES_BLOCK_SIZE (16)

psa_status_t cracen_cmac_setup(cracen_mac_operation_t *operation,
				      const psa_key_attributes_t *attributes,
				      const uint8_t *key_buffer, size_t key_buffer_size)
{
	int sx_status;
	psa_status_t status = PSA_SUCCESS;

	/* Only AES-CMAC is supported */
	if (psa_get_key_type(attributes) != PSA_KEY_TYPE_AES) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (key_buffer_size < AES_BLOCK_SIZE) {
		return PSA_ERROR_INVALID_ARGUMENT;
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
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* As only AES is supported it is always the same block size*/
	operation->bytes_left_for_next_block = AES_BLOCK_SIZE;
	operation->cmac.is_first_block = true;

	return PSA_SUCCESS;
}

psa_status_t cracen_cmac_update(cracen_mac_operation_t *operation, const uint8_t *input,
				       size_t input_length)
{
	int status;
	size_t block_bytes = 0;
	size_t remaining_bytes = 0;

	if (input_length <= operation->bytes_left_for_next_block) {
		/* Input is buffered to fill full AES blocks. */
		size_t offset = AES_BLOCK_SIZE - operation->bytes_left_for_next_block;

		memcpy(operation->input_buffer + offset, input, input_length);
		operation->bytes_left_for_next_block -= input_length;

		/* Can't fill a full block so nothing more to do here. */
		return PSA_SUCCESS;
	}

	/* The state can only be resumed if is not the first time data are
	 * processed.
	 */
	if (!operation->cmac.is_first_block) {
		status = sx_mac_resume_state(&operation->cmac.ctx);
		if (status) {
			return silex_statuscodes_to_psa(status);
		}
	}

	if (operation->bytes_left_for_next_block != AES_BLOCK_SIZE &&
	    operation->bytes_left_for_next_block != 0) {
		memcpy(operation->input_buffer +
			       (AES_BLOCK_SIZE - operation->bytes_left_for_next_block),
		       input, operation->bytes_left_for_next_block);
		input += operation->bytes_left_for_next_block;
		input_length -= operation->bytes_left_for_next_block;
		operation->bytes_left_for_next_block = 0;
	}

	/* Process the maximum number of full AES blocks. The processing of the
	 * last byte must be postponed until the operation is finished in @ref
	 * cracen_cmac_finish. This is because we need to feed the HW some data
	 * when generating the mac.
	 */
	block_bytes = ROUND_DOWN(input_length - 1, AES_BLOCK_SIZE);
	remaining_bytes = input_length - block_bytes;

	if (operation->bytes_left_for_next_block == 0) {
		status = sx_mac_feed(&operation->cmac.ctx, operation->input_buffer, AES_BLOCK_SIZE);
		if (status) {
			return silex_statuscodes_to_psa(status);
		}
		/* Input buffer was processed, reset the bytes for the next block and
		 * empty input buffer
		 */
		operation->bytes_left_for_next_block = AES_BLOCK_SIZE;
		operation->cmac.is_first_block = false;
	}

	if (block_bytes) {
		status = sx_mac_feed(&operation->cmac.ctx, input, block_bytes);
		if (status) {
			return silex_statuscodes_to_psa(status);
		}
		operation->cmac.is_first_block = false;
	}

	if (!operation->cmac.is_first_block) {
		/* save state and wait until processed */
		status = sx_mac_save_state(&operation->cmac.ctx);
		if (status) {
			return silex_statuscodes_to_psa(status);
		}
		status = sx_mac_wait(&operation->cmac.ctx);
		if (status) {
			return silex_statuscodes_to_psa(status);
		}

		/* Clear the input_buffer only after the sx_mac_wait call is executed
		 * since we need to make sure that the DMA transaction is finished
		 */
		safe_memzero(operation->input_buffer, sizeof(operation->input_buffer));
	}

	/* Copy the remaining bytes to the local input buffer */
	if (remaining_bytes > 0) {
		memcpy(operation->input_buffer, input + block_bytes, remaining_bytes);
		operation->bytes_left_for_next_block -= remaining_bytes;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_cmac_finish(cracen_mac_operation_t *operation)
{
	int status;

	if (operation->mac_size > AES_BLOCK_SIZE) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!operation->cmac.is_first_block) {
		status = sx_mac_resume_state(&operation->cmac.ctx);
		if (status) {
			return silex_statuscodes_to_psa(status);
		}
	}

	/* If there are data left in the input buffer feed them to the driver.
	 */
	if (operation->bytes_left_for_next_block != AES_BLOCK_SIZE) {
		status = sx_mac_feed(&operation->cmac.ctx, operation->input_buffer,
				     AES_BLOCK_SIZE - operation->bytes_left_for_next_block);
		if (status) {
			return silex_statuscodes_to_psa(status);
		}
	}

	/* Generate the MAC. */
	status = sx_mac_generate(&operation->cmac.ctx, operation->input_buffer);
	if (status) {
		return silex_statuscodes_to_psa(status);
	}
	status = sx_mac_wait(&operation->cmac.ctx);
	return silex_statuscodes_to_psa(status);
}
