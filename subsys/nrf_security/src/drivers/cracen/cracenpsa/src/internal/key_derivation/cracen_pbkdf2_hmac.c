/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/key_derivation/cracen_pbkdf2_hmac.h>
#include <internal/key_derivation/cracen_kdf_common.h>

#include <string.h>
#include <stdbool.h>
#include <silexpk/core.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>
#include <cracen_psa_primitives.h>

static size_t pbkdf2_prf_block_length(psa_algorithm_t alg)
{
	if (alg == PSA_ALG_PBKDF2_AES_CMAC_PRF_128) {
		return SX_BLKCIPHER_AES_BLK_SZ;
	} else {
		return PSA_HASH_BLOCK_LENGTH(PSA_ALG_GET_HASH(alg));
	}
}

static size_t pbkdf2_prf_output_length(psa_algorithm_t alg)
{
	if (alg == PSA_ALG_PBKDF2_AES_CMAC_PRF_128) {
		return SX_BLKCIPHER_AES_BLK_SZ;
	} else {
		return PSA_HASH_LENGTH(PSA_ALG_GET_HASH(alg));
	}
}

/**
 * \brief Generates the next block for PBKDF2.
 *
 * \param[in,out] operation  The key derivation operation.
 *
 * \return psa_status_t
 */
static psa_status_t pbkdf2_generate_block(cracen_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	unsigned int h_len = pbkdf2_prf_output_length(operation->alg);
	size_t mac_length = 0;

	operation->pbkdf2.blk_counter++;
	uint32_t blk_counter_be = uint32_to_be(operation->pbkdf2.blk_counter);

	/* Generate U1 = MAC(Password, Salt || BigEndian(i)) */
	status = cracen_kdf_start_mac_operation(operation, operation->pbkdf2.password,
						operation->pbkdf2.password_length);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_mac_update(&operation->mac_op, operation->pbkdf2.salt,
				   operation->pbkdf2.salt_length);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_mac_update(&operation->mac_op, (const uint8_t *)&blk_counter_be,
				   sizeof(blk_counter_be));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_mac_sign_finish(&operation->mac_op, operation->pbkdf2.uj,
					sizeof(operation->pbkdf2.uj), &mac_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	memcpy(operation->pbkdf2.tj, operation->pbkdf2.uj, h_len);

	for (uint64_t i = 1; i < operation->pbkdf2.input_cost; i++) {
		status = cracen_kdf_start_mac_operation(operation, operation->pbkdf2.password,
							operation->pbkdf2.password_length);
		if (status != PSA_SUCCESS) {
			return status;
		}

		status = cracen_mac_update(&operation->mac_op, operation->pbkdf2.uj, h_len);
		if (status != PSA_SUCCESS) {
			return status;
		}
		status = cracen_mac_sign_finish(&operation->mac_op, operation->pbkdf2.uj, h_len,
						&mac_length);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/* compute T_i = T_i ^ U_j */
		for (size_t i = 0; i < h_len; i++) {
			operation->pbkdf2.tj[i] = operation->pbkdf2.tj[i] ^ operation->pbkdf2.uj[i];
		}
	}

	memcpy(operation->output_block, operation->pbkdf2.tj, h_len);
	operation->output_block_available_bytes = h_len;

	return PSA_SUCCESS;
}

psa_status_t cracen_pbkdf2_hmac_setup(cracen_key_derivation_operation_t *operation)
{
	size_t output_length = pbkdf2_prf_output_length(operation->alg);

	if (output_length == 0) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	operation->capacity = (uint64_t)(UINT32_MAX)*output_length;
	operation->state = CRACEN_KD_STATE_PBKDF2_INIT;
	return PSA_SUCCESS;
}

psa_status_t cracen_pbkdf2_hmac_input_bytes(cracen_key_derivation_operation_t *operation,
					    psa_key_derivation_step_t step, const uint8_t *data,
					    size_t data_length)
{
	/* Operation must be initialized to a PBKDF2 state */
	if (!(operation->state & CRACEN_KD_STATE_PBKDF2_INIT)) {
		return PSA_ERROR_BAD_STATE;
	}

	/* No more input can be provided after we've started outputting data. */
	if (operation->state == CRACEN_KD_STATE_PBKDF2_OUTPUT) {
		return PSA_ERROR_BAD_STATE;
	}

	switch (step) {
	case PSA_KEY_DERIVATION_INPUT_SALT:
		if ((data_length + operation->pbkdf2.salt_length) >
		    sizeof(operation->pbkdf2.salt)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		/* Must provided one or more times. If used multiple times, the
		 * inputs will be concatenated.
		 */
		memcpy(operation->pbkdf2.salt + operation->pbkdf2.salt_length, data, data_length);
		operation->pbkdf2.salt_length += data_length;
		operation->state = CRACEN_KD_STATE_PBKDF2_SALT;
		break;

	case PSA_KEY_DERIVATION_INPUT_PASSWORD:
		/* Salt must have been provided. Password must be provided once.
		 */
		if (operation->state == CRACEN_KD_STATE_PBKDF2_PASSWORD ||
		    operation->state != CRACEN_KD_STATE_PBKDF2_SALT) {
			return PSA_ERROR_BAD_STATE;
		}

		size_t block_length = pbkdf2_prf_block_length(operation->alg);
		bool aes_cmac_prf = operation->alg == PSA_ALG_PBKDF2_AES_CMAC_PRF_128;

		if (aes_cmac_prf && data_length != block_length) {
			/* Password must be 128-bit (AES Key size) */
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		if (data_length > block_length) {
			size_t hash_length = 0;
			/* Password needs to be hashed. */
			psa_status_t status = cracen_hash_compute(
				PSA_ALG_HMAC_GET_HASH(operation->alg), data, data_length,
				operation->pbkdf2.password,
				PSA_HASH_LENGTH(PSA_ALG_GET_HASH(operation->alg)), &hash_length);
			operation->pbkdf2.password_length = hash_length;
			if (status != PSA_SUCCESS) {
				return status;
			}
		} else {
			memcpy(operation->pbkdf2.password, data, data_length);
			operation->pbkdf2.password_length = data_length;
		}

		operation->state = CRACEN_KD_STATE_PBKDF2_PASSWORD;
		break;

	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_pbkdf2_hmac_input_integer(cracen_key_derivation_operation_t *operation,
					      psa_key_derivation_step_t step, uint64_t value)
{
	if (step == PSA_KEY_DERIVATION_INPUT_COST) {
		if (operation->pbkdf2.input_cost) {
			/* Can only be provided once. */
			return PSA_ERROR_BAD_STATE;
		}
		operation->pbkdf2.input_cost = value;
		return PSA_SUCCESS;
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_pbkdf2_hmac_output_bytes(cracen_key_derivation_operation_t *operation,
					     uint8_t *output, size_t output_length)
{
	/* Salt, password and input cost must have been provided. */
	if (!operation->pbkdf2.input_cost) {
		return PSA_ERROR_BAD_STATE;
	}

	if (operation->state != CRACEN_KD_STATE_PBKDF2_PASSWORD &&
		operation->state != CRACEN_KD_STATE_PBKDF2_OUTPUT) {
		return PSA_ERROR_BAD_STATE;
	}

	operation->state = CRACEN_KD_STATE_PBKDF2_OUTPUT;
	return cracen_kdf_generate_output_bytes(operation, pbkdf2_generate_block,
						output, output_length);
}
