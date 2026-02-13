/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/key_derivation/cracen_tls12.h>
#include <internal/key_derivation/cracen_kdf_common.h>

#include <string.h>
#include <stdbool.h>
#include <silexpk/core.h>
#include <sxsymcrypt/hash.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>
#include <cracen_psa_primitives.h>

static psa_status_t tls12_input_bytes(cracen_key_derivation_operation_t *operation,
				      psa_key_derivation_step_t step, const uint8_t *data,
				      size_t data_length)
{
	/* Operation must be initialized to a TLS12 PRF state */
	if (operation->state != CRACEN_KD_STATE_TLS12_PRF_INIT &&
	    operation->state != CRACEN_KD_STATE_TLS12_PSK_TO_MS_INIT) {
		return PSA_ERROR_BAD_STATE;
	}

	/* Each input can only be provided once. */
	switch (step) {
	case PSA_KEY_DERIVATION_INPUT_SEED:
		if (data_length > sizeof(operation->tls12.seed)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		memcpy(operation->tls12.seed, data, data_length);
		operation->tls12.seed_length = data_length;
		break;
	case PSA_KEY_DERIVATION_INPUT_OTHER_SECRET:
		if (!PSA_ALG_IS_TLS12_PSK_TO_MS(operation->alg)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		if (data_length > (sizeof(operation->tls12.secret) - 4)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		/* First two bytes is uint16 that will be encoded in the mandatory
		 * PSA_KEY_DERIVATION_INPUT_SECRET step
		 */
		memcpy(operation->tls12.secret + 2, data, data_length);
		operation->tls12.secret_length = data_length;
		break;
	case PSA_KEY_DERIVATION_INPUT_SECRET:
		if (data_length > sizeof(operation->tls12.secret)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		if (PSA_ALG_IS_TLS12_PSK_TO_MS(operation->alg)) {
			if (data_length > PSA_TLS12_PSK_TO_MS_PSK_MAX_SIZE) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			size_t other_secret_length = operation->tls12.secret_length;
			/* Other secret not provided - plain PSK. */
			if (other_secret_length == 0) {
				memset(operation->tls12.secret, 0, data_length);
				other_secret_length = data_length;
			}
			operation->tls12.secret[0] = other_secret_length >> 8;
			operation->tls12.secret[1] = other_secret_length;
			operation->tls12.secret[other_secret_length + 2] = data_length >> 8;
			operation->tls12.secret[other_secret_length + 3] = data_length;
			memcpy(operation->tls12.secret + other_secret_length + 4, data,
			       data_length);
			operation->tls12.secret_length = other_secret_length + data_length + 4;
		} else if (PSA_ALG_IS_TLS12_PRF(operation->alg)) {
			memcpy(operation->tls12.secret, data, data_length);
			operation->tls12.secret_length = data_length;
		} else {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		break;
	case PSA_KEY_DERIVATION_INPUT_LABEL:
		if (data_length > sizeof(operation->tls12.label)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		memcpy(operation->tls12.label, data, data_length);
		operation->tls12.label_length = data_length;
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	return PSA_SUCCESS;
}

static psa_status_t tls12_prf_generate_block(cracen_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t length;

	const struct sxhashalg *hash;

	status = silex_statuscodes_to_psa(hash_get_algo(PSA_ALG_GET_HASH(operation->alg), &hash));
	if (status != PSA_SUCCESS) {
		return status;
	}
	size_t digest_size = sx_hash_get_alg_digestsz(hash);

	operation->tls12.counter++;

	/* A(i) = HMAC(A(i - 1)), A(0) = label || seed */
	status = cracen_kdf_start_mac_operation(operation, operation->tls12.secret,
						operation->tls12.secret_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (operation->tls12.counter == 1) {
		status = cracen_mac_update(&operation->mac_op, operation->tls12.label,
					   operation->tls12.label_length);
		if (status != PSA_SUCCESS) {
			return status;
		}
		status = cracen_mac_update(&operation->mac_op, operation->tls12.seed,
					   operation->tls12.seed_length); /* A(0) */
	} else {
		status = cracen_mac_update(&operation->mac_op, operation->tls12.a,
					   digest_size); /* A(1) */
	}
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_mac_sign_finish(&operation->mac_op, operation->tls12.a, digest_size,
					&length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* P_hash() = HMAC(A(i) || label || seed) */
	status = cracen_kdf_start_mac_operation(operation, operation->tls12.secret,
						operation->tls12.secret_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_mac_update(&operation->mac_op, operation->tls12.a, digest_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_mac_update(&operation->mac_op, operation->tls12.label,
				   operation->tls12.label_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_mac_update(&operation->mac_op, operation->tls12.seed,
				   operation->tls12.seed_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_mac_sign_finish(&operation->mac_op, operation->output_block, digest_size,
					&length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->output_block_available_bytes = digest_size;

	return status;
}

psa_status_t cracen_tls12_prf_setup(cracen_key_derivation_operation_t *operation)
{
	operation->state = CRACEN_KD_STATE_TLS12_PRF_INIT;
	operation->capacity = UINT64_MAX;
	return PSA_SUCCESS;
}

psa_status_t cracen_tls12_prf_input_bytes(cracen_key_derivation_operation_t *operation,
					  psa_key_derivation_step_t step,
					  const uint8_t *data, size_t data_length)
{
	return tls12_input_bytes(operation, step, data, data_length);
}

psa_status_t cracen_tls12_prf_output_bytes(cracen_key_derivation_operation_t *operation,
					   uint8_t *output, size_t output_length)
{
	operation->state = CRACEN_KD_STATE_TLS12_PRF_OUTPUT;
	return cracen_kdf_generate_output_bytes(operation, tls12_prf_generate_block,
						output, output_length);
}

psa_status_t cracen_tls12_psk_to_ms_setup(cracen_key_derivation_operation_t *operation)
{
	operation->state = CRACEN_KD_STATE_TLS12_PSK_TO_MS_INIT;
	operation->capacity = UINT64_MAX;
	return PSA_SUCCESS;
}

psa_status_t cracen_tls12_psk_to_ms_input_bytes(cracen_key_derivation_operation_t *operation,
						psa_key_derivation_step_t step,
						const uint8_t *data, size_t data_length)
{
	return tls12_input_bytes(operation, step, data, data_length);
}

psa_status_t cracen_tls12_psk_to_ms_output_bytes(cracen_key_derivation_operation_t *operation,
						 uint8_t *output, size_t output_length)
{
	operation->state = CRACEN_KD_STATE_TLS12_PSK_TO_MS_OUTPUT;
	return cracen_kdf_generate_output_bytes(operation, tls12_prf_generate_block,
						output, output_length);
}
