/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/key_derivation/cracen_hkdf.h>
#include <internal/key_derivation/cracen_kdf_common.h>

#include <string.h>
#include <stdbool.h>
#include <silexpk/core.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>
#include <cracen_psa_primitives.h>

static psa_status_t hkdf_input_bytes(cracen_key_derivation_operation_t *operation,
				     psa_key_derivation_step_t step, const uint8_t *data,
				     size_t data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	/* Operation must be initialized to a HKDF state */
	if (!(operation->state & CRACEN_KD_STATE_HKDF_INIT)) {
		return PSA_ERROR_BAD_STATE;
	}

	/* No more input can be provided after we've started outputting data. */
	if (operation->state == CRACEN_KD_STATE_HKDF_OUTPUT) {
		return PSA_ERROR_BAD_STATE;
	}

	switch (step) {
	case PSA_KEY_DERIVATION_INPUT_SALT:
		/* This must be the first input */
		if (operation->state != CRACEN_KD_STATE_HKDF_INIT) {
			return PSA_ERROR_BAD_STATE;
		}
		status = cracen_kdf_start_mac_operation(operation, data, data_length);
		if (status != PSA_SUCCESS) {
			return status;
		}
		operation->state = CRACEN_KD_STATE_HKDF_STARTED;
		break;

	case PSA_KEY_DERIVATION_INPUT_SECRET: {
		size_t hmac_length = 0;

		if (operation->state == CRACEN_KD_STATE_HKDF_INIT) {
			/* rfc5869: salt is optional. Set to string of HashLen
			 * zeroes if not provided.
			 * We reuse prk from the operation context since it's
			 * not in use yet.
			 */
			safe_memzero(operation->hkdf.prk, sizeof(operation->hkdf.prk));
			status = cracen_kdf_start_mac_operation(
				operation, operation->hkdf.prk,
				PSA_HASH_BLOCK_LENGTH(PSA_ALG_GET_HASH(operation->alg)));
			if (status != PSA_SUCCESS) {
				return status;
			}
		} else if (operation->state != CRACEN_KD_STATE_HKDF_STARTED) {
			return PSA_ERROR_BAD_STATE;
		} else {
			/* For compliance */
		}

		status = cracen_mac_update(&operation->mac_op, data, data_length);
		if (status != PSA_SUCCESS) {
			return status;
		}
		operation->state = CRACEN_KD_STATE_HKDF_KEYED;
		return cracen_mac_sign_finish(
			&operation->mac_op, operation->hkdf.prk,
			PSA_HASH_BLOCK_LENGTH(PSA_ALG_GET_HASH(operation->alg)), &hmac_length);
	}

	case PSA_KEY_DERIVATION_INPUT_INFO:
		if (operation->state == CRACEN_KD_STATE_HKDF_OUTPUT) {
			return PSA_ERROR_BAD_STATE;
		}
		if (operation->hkdf.info_set) {
			return PSA_ERROR_BAD_STATE;
		}

		if (data_length > sizeof(operation->hkdf.info)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}

		memcpy(operation->hkdf.info, data, data_length);
		operation->hkdf.info_length = data_length;
		operation->hkdf.info_set = true;

		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

/**
 * \brief Generates the next block for HKDF.
 *
 * \param[in,out] operation  The key derivation operation.
 *
 * \return psa_status_t
 */
static psa_status_t hkdf_generate_block(cracen_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	unsigned int digestsz = PSA_HASH_LENGTH(PSA_ALG_GET_HASH(operation->alg));
	size_t hmac_length = 0;

	/* Create T(N) = HMAC-Hash(PRK, T(N-1) || info || N) */
	status = cracen_kdf_start_mac_operation(operation, operation->hkdf.prk, digestsz);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (operation->hkdf.blk_counter) {
		/* T(0) is empty. */
		status = cracen_mac_update(&operation->mac_op, operation->hkdf.t, digestsz);
		if (status != PSA_SUCCESS) {
			return status;
		}
	}

	operation->hkdf.blk_counter++;

	status = cracen_mac_update(&operation->mac_op, operation->hkdf.info,
				   operation->hkdf.info_length);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_mac_update(&operation->mac_op, &operation->hkdf.blk_counter,
				   sizeof(operation->hkdf.blk_counter));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_mac_sign_finish(&operation->mac_op, operation->hkdf.t, digestsz,
					&hmac_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	memcpy(operation->output_block, operation->hkdf.t, digestsz);
	operation->output_block_available_bytes = digestsz;

	return PSA_SUCCESS;
}

psa_status_t cracen_hkdf_setup(cracen_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (PSA_ALG_IS_HKDF(operation->alg) || PSA_ALG_IS_HKDF_EXPAND(operation->alg)) {
		size_t hash_size = PSA_HASH_LENGTH(PSA_ALG_HKDF_GET_HASH(operation->alg));

		if (hash_size == 0) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		operation->capacity =
			UINT8_MAX * hash_size; /* Max value of counter (1 byte) size of hash. */
		operation->state = CRACEN_KD_STATE_HKDF_INIT;

		status = PSA_SUCCESS;
	} else if (PSA_ALG_IS_HKDF_EXTRACT(operation->alg)) {
		size_t hash_size = PSA_HASH_LENGTH(PSA_ALG_HKDF_GET_HASH(operation->alg));

		if (hash_size == 0) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		operation->capacity =
			UINT8_MAX * hash_size; /* Max value of counter (1 byte) size of hash. */
		operation->state = CRACEN_KD_STATE_HKDF_INIT;

		status = PSA_SUCCESS;
	} else {
		status = PSA_ERROR_NOT_SUPPORTED;
	}

	return status;
}

psa_status_t cracen_hkdf_input_bytes(cracen_key_derivation_operation_t *operation,
				     psa_key_derivation_step_t step, const uint8_t *data,
				     size_t data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (PSA_ALG_IS_HKDF(operation->alg) || PSA_ALG_IS_HKDF_EXTRACT(operation->alg)) {
		status = hkdf_input_bytes(operation, step, data, data_length);
	} else if (PSA_ALG_IS_HKDF_EXPAND(operation->alg)) {
		if (step == PSA_KEY_DERIVATION_INPUT_SECRET) {
			if (data_length > sizeof(operation->hkdf.prk)) {
				return PSA_ERROR_INSUFFICIENT_MEMORY;
			}
			memcpy(operation->hkdf.prk, data, data_length);
			operation->state = CRACEN_KD_STATE_HKDF_KEYED;
			return PSA_SUCCESS;
		}
		status = hkdf_input_bytes(operation, step, data, data_length);
	} else {
		status = PSA_ERROR_NOT_SUPPORTED;
	}

	return status;
}

psa_status_t cracen_hkdf_output_bytes(cracen_key_derivation_operation_t *operation,
				      uint8_t *output, size_t output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (PSA_ALG_IS_HKDF(operation->alg) || PSA_ALG_IS_HKDF_EXPAND(operation->alg)) {
		if (operation->state < CRACEN_KD_STATE_HKDF_KEYED || !operation->hkdf.info_set) {
			return PSA_ERROR_BAD_STATE;
		}

		operation->state = CRACEN_KD_STATE_HKDF_OUTPUT;

		return cracen_kdf_generate_output_bytes(operation, hkdf_generate_block,
							output, output_length);

	} else if (PSA_ALG_IS_HKDF_EXTRACT(operation->alg)) {
		if (operation->state < CRACEN_KD_STATE_HKDF_KEYED) {
			return PSA_ERROR_BAD_STATE;
		}

		operation->state = CRACEN_KD_STATE_HKDF_OUTPUT;

		size_t prk_length = PSA_HASH_LENGTH(PSA_ALG_HKDF_GET_HASH(operation->alg));

		if (output_length < prk_length) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		memcpy(output, operation->hkdf.prk, prk_length);
		return PSA_SUCCESS;

	} else {
		status = PSA_ERROR_NOT_SUPPORTED;
	}
	return status;
}

psa_status_t cracen_hkdf_abort(cracen_key_derivation_operation_t *operation)
{
	return cracen_mac_abort(&operation->mac_op);
}
