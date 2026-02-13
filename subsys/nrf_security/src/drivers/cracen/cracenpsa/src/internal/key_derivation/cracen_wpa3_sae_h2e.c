/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/key_derivation/cracen_wpa3_sae_h2e.h>
#include <internal/key_derivation/cracen_kdf_common.h>

#include <string.h>
#include <stdbool.h>
#include <silexpk/core.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>
#include <cracen_psa_primitives.h>

psa_status_t cracen_wpa3_sae_h2e_setup(cracen_key_derivation_operation_t *operation)
{
	operation->state = CRACEN_KD_STATE_WPA3_SAE_H2E_INIT;
	return PSA_SUCCESS;
}

psa_status_t cracen_wpa3_sae_h2e_input_bytes(cracen_key_derivation_operation_t *operation,
					     psa_key_derivation_step_t step, const uint8_t *data,
					     size_t data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	/* Operation must be initialized to a WPA3-SAE H2E state */
	if (!(operation->state & CRACEN_KD_STATE_WPA3_SAE_H2E_INIT)) {
		return PSA_ERROR_BAD_STATE;
	}

	/* No more input can be provided after we've started outputting data. */
	if (operation->state == CRACEN_KD_STATE_WPA3_SAE_H2E_OUTPUT) {
		return PSA_ERROR_BAD_STATE;
	}

	switch (step) {
	case PSA_KEY_DERIVATION_INPUT_SALT:
		/* This must be the first input (SSID is a salt) */
		if (operation->state != CRACEN_KD_STATE_WPA3_SAE_H2E_INIT) {
			return PSA_ERROR_BAD_STATE;
		}

		status = cracen_kdf_start_mac_operation(operation, data, data_length);
		if (status != PSA_SUCCESS) {
			return status;
		}

		operation->state = CRACEN_KD_STATE_WPA3_SAE_H2E_SALT;
		break;

	case PSA_KEY_DERIVATION_INPUT_PASSWORD:
		/* Input password */
		if (operation->state != CRACEN_KD_STATE_WPA3_SAE_H2E_SALT) {
			return PSA_ERROR_BAD_STATE;
		}

		status = cracen_mac_update(&operation->mac_op, data, data_length);
		if (status != PSA_SUCCESS) {
			return status;
		}

		operation->state = CRACEN_KD_STATE_WPA3_SAE_H2E_PASSWORD;
		break;

	case PSA_KEY_DERIVATION_INPUT_INFO:
		/* PWID (password identifier) setting is optional */
		if (operation->state != CRACEN_KD_STATE_WPA3_SAE_H2E_PASSWORD) {
			return PSA_ERROR_BAD_STATE;
		}

		status = cracen_mac_update(&operation->mac_op, data, data_length);
		if (status != PSA_SUCCESS) {
			return status;
		}

		operation->state = CRACEN_KD_STATE_WPA3_SAE_H2E_INFO;
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_wpa3_sae_h2e_output_bytes(cracen_key_derivation_operation_t *operation,
					      uint8_t *output, size_t output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t outlen = 0;

	if (operation->state != CRACEN_KD_STATE_WPA3_SAE_H2E_PASSWORD &&
		operation->state != CRACEN_KD_STATE_WPA3_SAE_H2E_INFO) {
		return PSA_ERROR_BAD_STATE;
	}
	operation->state = CRACEN_KD_STATE_WPA3_SAE_H2E_OUTPUT;
	status = cracen_mac_sign_finish(&operation->mac_op, output, output_length, &outlen);

	if (output_length != outlen) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return status;
}

psa_status_t cracen_wpa3_sae_h2e_abort(cracen_key_derivation_operation_t *operation)
{
	return cracen_mac_abort(&operation->mac_op);
}
