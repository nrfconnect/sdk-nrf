/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <sicrypto/hmac.h>
#include <sicrypto/sicrypto.h>
#include <string.h>
#include <sxsymcrypt/cmac.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/keyref.h>
#include <zephyr/sys/__assert.h>
#include "common.h"
#include <cracen/mem_helpers.h>
#include "cracen_psa_primitives.h"

#define AES_BLOCK_SIZE (16)

static psa_status_t cracen_hmac_setup(cracen_mac_operation_t *operation,
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
		     MAX_HASH_BLOCK_SIZE + PSA_HASH_MAX_SIZE);
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

static psa_status_t cracen_hmac_finish(cracen_mac_operation_t *operation)
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

static psa_status_t cracen_cmac_setup(cracen_mac_operation_t *operation,
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

	return status;
}

static psa_status_t cracen_cmac_finish(cracen_mac_operation_t *operation)
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
	if (status) {
		return silex_statuscodes_to_psa(status);
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_mac_setup(cracen_mac_operation_t *operation,
			      const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			      size_t key_buffer_size, psa_algorithm_t alg)
{
	/* Assuming that psa_core checks that key has PSA_KEY_USAGE_SIGN_MESSAGE
	 * set
	 * Assuming that psa_core checks that alg is PSA_ALG_IS_MAC(alg) == true
	 */
	__ASSERT_NO_MSG(PSA_ALG_IS_MAC(alg));

	/* Operation must be empty */
	if (operation->alg != 0) {
		return PSA_ERROR_BAD_STATE;
	}

	operation->alg = alg;
	operation->mac_size =
		PSA_MAC_LENGTH(psa_get_key_type(attributes), psa_get_key_bits(attributes), alg);

	if (IS_ENABLED(PSA_NEED_CRACEN_HMAC)) {
		if (PSA_ALG_IS_HMAC(alg)) {
			return cracen_hmac_setup(operation, attributes, key_buffer, key_buffer_size,
						 alg);
		}
	}
	if (IS_ENABLED(PSA_NEED_CRACEN_CMAC)) {
		if (PSA_ALG_FULL_LENGTH_MAC(alg) == PSA_ALG_CMAC) {
			return cracen_cmac_setup(operation, attributes, key_buffer,
						 key_buffer_size);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_mac_sign_setup(cracen_mac_operation_t *operation,
				   const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg)
{
	return cracen_mac_setup(operation, attributes, key_buffer, key_buffer_size, alg);
}

psa_status_t cracen_mac_verify_setup(cracen_mac_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg)
{
	return cracen_mac_setup(operation, attributes, key_buffer, key_buffer_size, alg);
}

static psa_status_t cracen_hmac_update(cracen_mac_operation_t *operation, const uint8_t *input,
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
	input_chunk_length += (input_length - input_chunk_length) & ~(block_size - 1);
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

psa_status_t cracen_mac_update(cracen_mac_operation_t *operation, const uint8_t *input,
			       size_t input_length)
{
	if (operation->alg == 0) {
		return PSA_ERROR_BAD_STATE;
	}

	/* Valid PSA call, just nothing to do. */
	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_HMAC)) {
		if (PSA_ALG_IS_HMAC(operation->alg)) {
			return cracen_hmac_update(operation, input, input_length);
		}
	}
	if (IS_ENABLED(PSA_NEED_CRACEN_CMAC)) {
		if (PSA_ALG_FULL_LENGTH_MAC(operation->alg) == PSA_ALG_CMAC) {
			return cracen_cmac_update(operation, input, input_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_mac_sign_finish(cracen_mac_operation_t *operation, uint8_t *mac,
				    size_t mac_size, size_t *mac_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (operation->alg == 0) {
		return PSA_ERROR_BAD_STATE;
	}

	if (mac == NULL || mac_size == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (mac_size < operation->mac_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_HMAC)) {
		if (PSA_ALG_IS_HMAC(operation->alg)) {
			status = cracen_hmac_finish(operation);
		}
	}
	if (IS_ENABLED(PSA_NEED_CRACEN_CMAC)) {
		if (PSA_ALG_FULL_LENGTH_MAC(operation->alg) == PSA_ALG_CMAC) {
			status = cracen_cmac_finish(operation);
		}
	}

	if (status != PSA_SUCCESS) {
		*mac_length = 0;
		return status;
	}

	/* Copy out the from out internal buffer to output buffer. Truncation
	 * can happen here.
	 */
	memcpy(mac, operation->input_buffer, operation->mac_size);
	*mac_length = operation->mac_size;

	safe_memzero((void *)operation, sizeof(cracen_mac_operation_t));

	return PSA_SUCCESS;
}

psa_status_t cracen_mac_verify_finish(cracen_mac_operation_t *operation, const uint8_t *mac,
				      size_t mac_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (operation->alg == 0) {
		return PSA_ERROR_BAD_STATE;
	}

	if (mac == NULL || mac_length == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* If the provided buffer is of a different size than the calculated
	 * one, it will not match.
	 */
	if (mac_length != operation->mac_size) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_HMAC)) {
		if (PSA_ALG_IS_HMAC(operation->alg)) {
			status = cracen_hmac_finish(operation);
		}
	}
	if (IS_ENABLED(PSA_NEED_CRACEN_CMAC)) {
		if (PSA_ALG_FULL_LENGTH_MAC(operation->alg) == PSA_ALG_CMAC) {
			status = cracen_cmac_finish(operation);
		}
	}

	if (status != PSA_SUCCESS) {
		return status;
	}

	/* Do a constant time mem compare. */
	status = constant_memcmp(operation->input_buffer, mac, operation->mac_size);
	if (status) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}

	safe_memzero((void *)operation, sizeof(cracen_mac_operation_t));

	return PSA_SUCCESS;
}

psa_status_t cracen_mac_compute(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, uint8_t *mac, size_t mac_size,
				size_t *mac_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	cracen_mac_operation_t operation = {0};

	status = cracen_mac_setup(&operation, attributes, key_buffer, key_buffer_size, alg);
	if (status != PSA_SUCCESS) {
		cracen_mac_abort(&operation);
		return status;
	}

	status = cracen_mac_update(&operation, input, input_length);
	if (status != PSA_SUCCESS) {
		cracen_mac_abort(&operation);
		return status;
	}

	status = cracen_mac_sign_finish(&operation, mac, mac_size, mac_length);
	if (status != PSA_SUCCESS) {
		cracen_mac_abort(&operation);
		return status;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_mac_abort(cracen_mac_operation_t *operation)
{
	safe_memzero((void *)operation, sizeof(cracen_mac_operation_t));

	return PSA_SUCCESS;
}
