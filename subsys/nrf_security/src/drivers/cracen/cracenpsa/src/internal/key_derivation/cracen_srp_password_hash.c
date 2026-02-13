/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/key_derivation/cracen_srp_password_hash.h>
#include <internal/key_derivation/cracen_kdf_common.h>

#include <string.h>
#include <stdbool.h>
#include <silexpk/core.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>
#include <cracen_psa_primitives.h>

psa_status_t cracen_srp_password_hash_setup(cracen_key_derivation_operation_t *operation)
{
	if (PSA_ALG_HKDF_GET_HASH(operation->alg) != CRACEN_SRP_HASH_ALG) {
		return PSA_ERROR_NOT_SUPPORTED;
	}
	operation->capacity = CRACEN_SRP_HASH_LENGTH;
	return PSA_SUCCESS;
}

psa_status_t cracen_srp_password_hash_input_bytes(cracen_key_derivation_operation_t *operation,
						  psa_key_derivation_step_t step,
						  const uint8_t *data, size_t data_length)
{
	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	size_t output_length = 0;

	/* Each input can only be provided once. */
	switch (step) {
	case PSA_KEY_DERIVATION_INPUT_INFO:
		status = cracen_hash_setup(&operation->hash_op, CRACEN_SRP_HASH_ALG);
		if (status != PSA_SUCCESS) {
			break;
		}
		/* Add the username */
		status = cracen_hash_update(&operation->hash_op, data, data_length);
		break;

	case PSA_KEY_DERIVATION_INPUT_PASSWORD:
		/* Add the character ':' before the password */
		status = cracen_hash_update(&operation->hash_op, (const uint8_t *)":", 1);
		if (status != PSA_SUCCESS) {
			break;
		}
		status = cracen_hash_update(&operation->hash_op, data, data_length);
		break;

	case PSA_KEY_DERIVATION_INPUT_SALT:
		if (data_length > 0) {
			status =
				cracen_hash_finish(&operation->hash_op, operation->output_block,
						   sizeof(operation->output_block), &output_length);
			if (status != PSA_SUCCESS) {
				break;
			}
			status = cracen_hash_setup(&operation->hash_op, CRACEN_SRP_HASH_ALG);
			if (status != PSA_SUCCESS) {
				break;
			}
			/* Add the salt */
			status = cracen_hash_update(&operation->hash_op, data, data_length);
			if (status != PSA_SUCCESS) {
				break;
			}
			/* Finally add the previous hash which is H(username | : | password ) */
			status = cracen_hash_update(&operation->hash_op, operation->output_block,
						    output_length);
		}
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (status != PSA_SUCCESS) {
		cracen_hash_abort(&operation->hash_op);
	}
	return status;
}

psa_status_t cracen_srp_password_hash_output_bytes(cracen_key_derivation_operation_t *operation,
						   uint8_t *output, size_t output_length)
{
	size_t outlen = 0;
	psa_status_t status =
		cracen_hash_finish(&operation->hash_op, output, output_length, &outlen);
	if (status != PSA_SUCCESS) {
		cracen_hash_abort(&operation->hash_op);
	}

	if (output_length != outlen) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return status;
}

psa_status_t cracen_srp_password_hash_abort(cracen_key_derivation_operation_t *operation)
{
	return cracen_hash_abort(&operation->hash_op);
}
