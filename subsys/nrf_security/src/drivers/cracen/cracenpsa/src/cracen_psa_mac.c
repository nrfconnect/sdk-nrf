/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa_mac.h>

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <cracen/common.h>
#include <cracen/mem_helpers.h>
#include "cracen_psa_primitives.h"
#include <internal/mac/cracen_mac_cmac.h>
#include <internal/mac/cracen_mac_hmac.h>

static psa_status_t setup(cracen_mac_operation_t *operation, const psa_key_attributes_t *attributes,
			  const uint8_t *key_buffer, size_t key_buffer_size, psa_algorithm_t alg)
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
	return setup(operation, attributes, key_buffer, key_buffer_size, alg);
}

psa_status_t cracen_mac_verify_setup(cracen_mac_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg)
{
	return setup(operation, attributes, key_buffer, key_buffer_size, alg);
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

	if (mac == NULL) {
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

	return cracen_mac_abort(operation);
}

psa_status_t cracen_mac_verify_finish(cracen_mac_operation_t *operation, const uint8_t *mac,
				      size_t mac_size)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (operation->alg == 0) {
		return PSA_ERROR_BAD_STATE;
	}

	if (mac == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* If the provided buffer is of a different size than the calculated
	 * one, it will not match.
	 */
	if (mac_size != operation->mac_size) {
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
	if (status != SX_OK) {
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
	psa_status_t status;
	cracen_mac_operation_t operation = {0};

	status = setup(&operation, attributes, key_buffer, key_buffer_size, alg);
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	status = cracen_mac_update(&operation, input, input_length);
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	status = cracen_mac_sign_finish(&operation, mac, mac_size, mac_length);
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	return PSA_SUCCESS;

error_exit:
	cracen_mac_abort(&operation);
	return status;
}

psa_status_t cracen_mac_abort(cracen_mac_operation_t *operation)
{
	safe_memzero((void *)operation, sizeof(cracen_mac_operation_t));

	return PSA_SUCCESS;
}
