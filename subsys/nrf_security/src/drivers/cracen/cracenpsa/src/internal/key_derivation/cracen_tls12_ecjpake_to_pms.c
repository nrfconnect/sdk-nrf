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
#include <cracen_psa_hash.h>

psa_status_t cracen_tls12_ecjpake_to_pms_setup(cracen_key_derivation_operation_t *operation)
{
	operation->capacity = PSA_TLS12_ECJPAKE_TO_PMS_DATA_SIZE;
	operation->state = CRACEN_KD_STATE_TLS12_ECJPAKE_TO_PMS_INIT;
	return PSA_SUCCESS;
}

psa_status_t cracen_tls12_ecjpake_to_pms_input_bytes(cracen_key_derivation_operation_t *operation,
						     psa_key_derivation_step_t step,
						     const uint8_t *data, size_t data_length)
{
	if (operation->state != CRACEN_KD_STATE_TLS12_ECJPAKE_TO_PMS_INIT) {
		return PSA_ERROR_BAD_STATE;
	}
	operation->state = CRACEN_KD_STATE_TLS12_ECJPAKE_TO_PMS_OUTPUT;
	if (data_length != 65 || data[0] != 0x04) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	/* Copy the x coordinate of the point. */
	memcpy(operation->ecjpake_to_pms.key, data + 1,
		sizeof(operation->ecjpake_to_pms.key));
	return PSA_SUCCESS;
}

psa_status_t cracen_tls12_ecjpake_to_pms_output_bytes(cracen_key_derivation_operation_t *operation,
						      uint8_t *output, size_t output_length)
{
	size_t outlen;
	psa_status_t status = cracen_hash_compute(
		PSA_ALG_SHA_256, operation->ecjpake_to_pms.key, 32, output, 32, &outlen);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (outlen != 32) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}
	return PSA_SUCCESS;
}
