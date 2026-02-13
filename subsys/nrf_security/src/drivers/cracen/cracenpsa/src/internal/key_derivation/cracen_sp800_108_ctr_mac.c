/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/key_derivation/cracen_sp800_108_ctr_mac.h>
#include <internal/key_derivation/cracen_kdf_common.h>

#include <string.h>
#include <stdbool.h>
#include <silexpk/core.h>
#include <sxsymcrypt/hash.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>
#include <cracen_psa_primitives.h>
#include <cracen_psa_mac_kdf.h>

static psa_status_t
cracen_ctr_mac_add_core_fixed_input(cracen_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	/* Make sure the byte after the label is set to zero */
	safe_memzero(operation->mac_ctr.label + operation->mac_ctr.label_length, 1);

	/* Label + 0x00*/
	status = cracen_mac_update(&operation->mac_op, operation->mac_ctr.label,
				   operation->mac_ctr.label_length + 1);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* Context */
	status = cracen_mac_update(&operation->mac_op, operation->mac_ctr.context,
				   operation->mac_ctr.context_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* L_4 */
	status = cracen_mac_update(&operation->mac_op, (const uint8_t *)&operation->mac_ctr.L,
				   sizeof(operation->mac_ctr.L));
	return status;
}

static psa_status_t cracen_ctr_cmac_generate_K_0(cracen_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t length;

	/* The capacity changes when the output bytes are derived, but L must not change, therefore
	 * saving it separately
	 */
	operation->mac_ctr.L = uint32_to_be(PSA_BYTES_TO_BITS(operation->capacity));

	status = cracen_kdf_start_mac_operation(operation, operation->mac_ctr.key_buffer,
						operation->mac_ctr.key_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_ctr_mac_add_core_fixed_input(operation);
	if (status) {
		return status;
	}

	status = cracen_mac_sign_finish(&operation->mac_op, operation->mac_ctr.K_0,
					SX_BLKCIPHER_AES_BLK_SZ, &length);
	return status;
}

/**
 * @brief Generate a PRF block based on CMAC in counter mode (NIST.SP.800-108r1)
 *
 * Here are the parameters of this implementation:
 * r = 32 bits (counter is a uint32_t)
 * h = 128 bits (since we use CMAC the output block is 128 bit long)
 *
 * The algorithm specifies L as the bit length of the requested data length but the
 * PSA APIS don't support setting the length of the requested output so here we
 * always set L = 128 bits since we always output a single block.
 *
 */
static psa_status_t cracen_ctr_mac_generate_block(cracen_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint32_t counter_be;
	size_t length;
	size_t mac_sz = SX_BLKCIPHER_AES_BLK_SZ;

#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC)
	if (PSA_ALG_IS_SP800_108_COUNTER_HMAC(operation->alg)) {
		const struct sxhashalg *hash;

		status = hash_get_algo(PSA_ALG_GET_HASH(operation->alg), &hash);
		if (status != PSA_SUCCESS) {
			return status;
		}
		mac_sz = sx_hash_get_alg_digestsz(hash);
	}
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC */

#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC)
	if (operation->alg == PSA_ALG_SP800_108_COUNTER_CMAC) {
		status = cracen_kdf_start_mac_operation(operation, operation->mac_ctr.key_buffer,
							operation->mac_ctr.key_size);
		if (status != PSA_SUCCESS) {
			return status;
		}
	}
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC */

	counter_be = uint32_to_be(operation->mac_ctr.counter);
	status = cracen_mac_update(&operation->mac_op, (const uint8_t *)&counter_be,
				   sizeof(counter_be));
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_ctr_mac_add_core_fixed_input(operation);
	if (status) {
		return status;
	}

#if defined(PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC)
	if (operation->alg == PSA_ALG_SP800_108_COUNTER_CMAC) {
		status = cracen_mac_update(&operation->mac_op, operation->mac_ctr.K_0,
					   sizeof(operation->mac_ctr.K_0));
		if (status != PSA_SUCCESS) {
			return status;
		}
	}
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC */

	status = cracen_mac_sign_finish(&operation->mac_op, operation->output_block,
					mac_sz, &length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->output_block_available_bytes = mac_sz;
	operation->mac_ctr.counter++;
	return PSA_SUCCESS;
}

psa_status_t cracen_sp800_108_ctr_mac_setup(cracen_key_derivation_operation_t *operation)
{
	operation->capacity = PSA_ALG_SP800_108_COUNTER_MAC_INIT_CAPACITY;
	operation->state = CRACEN_KD_STATE_MAC_CTR_INIT;
	/* HMAC and CMAC CTR key derivation starts the counter with 1, see NIST.SP.800-108r1 */
	operation->mac_ctr.counter = 1;

	return PSA_SUCCESS;
}

psa_status_t cracen_sp800_108_ctr_mac_input_bytes(cracen_key_derivation_operation_t *operation,
						  psa_key_derivation_step_t step,
						  const uint8_t *data, size_t data_length)
{
	switch (step) {
	case PSA_KEY_DERIVATION_INPUT_LABEL: {
		if (operation->state != CRACEN_KD_STATE_MAC_CTR_KEY_LOADED) {
			return PSA_ERROR_BAD_STATE;
		}

		/* Reserve the last byte of the label for setting the byte 0x0 which is required
		 * by the CMAC CTR key derivation.
		 */
		size_t label_remaining_bytes = sizeof(operation->mac_ctr.label) - 1;

		if (data_length > label_remaining_bytes) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}

		memcpy(operation->mac_ctr.label, data, data_length);
		operation->mac_ctr.label_length = data_length;

		operation->state = CRACEN_KD_STATE_MAC_CTR_INPUT_LABEL;
		break;
	}
	case PSA_KEY_DERIVATION_INPUT_CONTEXT: {
		if (operation->state != CRACEN_KD_STATE_MAC_CTR_KEY_LOADED &&
		    operation->state != CRACEN_KD_STATE_MAC_CTR_INPUT_LABEL) {
			return PSA_ERROR_BAD_STATE;
		}

		if (data_length > sizeof(operation->mac_ctr.context)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}

		memcpy(operation->mac_ctr.context, data, data_length);
		operation->mac_ctr.context_length = data_length;

		operation->state = CRACEN_KD_STATE_MAC_CTR_INPUT_CONTEXT;
		break;
	}

	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_sp800_108_ctr_cmac_input_key(cracen_key_derivation_operation_t *operation,
						 psa_key_derivation_step_t step,
						 const psa_key_attributes_t *attributes,
						 const uint8_t *key_buffer, size_t key_buffer_size)
{
	if (psa_get_key_type(attributes) != PSA_KEY_TYPE_AES) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (operation->state != CRACEN_KD_STATE_MAC_CTR_INIT ||
	    step != PSA_KEY_DERIVATION_INPUT_SECRET) {
		return PSA_ERROR_BAD_STATE;
	}

	/**
	 * Storing key attributes here since the persistent key can be used.
	 * In this case key_buffer_size is 0.
	 */
	operation->mac_ctr.key_lifetime = psa_get_key_lifetime(attributes);
	operation->mac_ctr.key_id = psa_get_key_id(attributes);

	/*
	 * Copy the key into the operation struct as it is not guaranteed
	 * to be valid longer than the function call.
	 */
	if (key_buffer_size > sizeof(operation->mac_ctr.key_buffer)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	memcpy(operation->mac_ctr.key_buffer, key_buffer, key_buffer_size);
	operation->mac_ctr.key_size = key_buffer_size;

	operation->state = CRACEN_KD_STATE_MAC_CTR_KEY_LOADED;
	return PSA_SUCCESS;
}

psa_status_t cracen_sp800_108_ctr_hmac_input_key(cracen_key_derivation_operation_t *operation,
						 psa_key_derivation_step_t step,
						 const psa_key_attributes_t *attributes,
						 const uint8_t *key_buffer, size_t key_buffer_size)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (psa_get_key_type(attributes) != PSA_KEY_TYPE_HMAC) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (operation->state != CRACEN_KD_STATE_MAC_CTR_INIT ||
	    step != PSA_KEY_DERIVATION_INPUT_SECRET) {
		return PSA_ERROR_BAD_STATE;
	}

	status = cracen_kdf_start_mac_operation(operation, key_buffer, key_buffer_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->state = CRACEN_KD_STATE_MAC_CTR_KEY_LOADED;
	return PSA_SUCCESS;
}

psa_status_t cracen_sp800_108_ctr_cmac_output_bytes(cracen_key_derivation_operation_t *operation,
						    uint8_t *output, size_t output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (operation->state == CRACEN_KD_STATE_MAC_CTR_KEY_LOADED ||
		operation->state == CRACEN_KD_STATE_MAC_CTR_INPUT_LABEL ||
		operation->state == CRACEN_KD_STATE_MAC_CTR_INPUT_CONTEXT ||
		operation->state == CRACEN_KD_STATE_MAC_CTR_OUTPUT) {

		if (operation->state != CRACEN_KD_STATE_MAC_CTR_OUTPUT) {
			operation->state = CRACEN_KD_STATE_MAC_CTR_OUTPUT;
			status = cracen_ctr_cmac_generate_K_0(operation);

			if (status != PSA_SUCCESS) {
				return status;
			}
		}

		status = cracen_kdf_generate_output_bytes(operation,
							  cracen_ctr_mac_generate_block,
							  output, output_length);

	} else {
		status = PSA_ERROR_BAD_STATE;
	}

	return status;
}

psa_status_t cracen_sp800_108_ctr_hmac_output_bytes(cracen_key_derivation_operation_t *operation,
						    uint8_t *output, size_t output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (operation->state == CRACEN_KD_STATE_MAC_CTR_KEY_LOADED ||
		operation->state == CRACEN_KD_STATE_MAC_CTR_INPUT_LABEL ||
		operation->state == CRACEN_KD_STATE_MAC_CTR_INPUT_CONTEXT ||
		operation->state == CRACEN_KD_STATE_MAC_CTR_OUTPUT) {

		if (operation->state != CRACEN_KD_STATE_MAC_CTR_OUTPUT) {
			operation->state = CRACEN_KD_STATE_MAC_CTR_OUTPUT;

			/* The capacity changes when the output bytes are derived,
			 * but L must not change, therefore saving it separately
			 */
			operation->mac_ctr.L =
				uint32_to_be(PSA_BYTES_TO_BITS(operation->capacity));
		}

		status = cracen_kdf_generate_output_bytes(operation,
							  cracen_ctr_mac_generate_block,
							  output, output_length);

	} else {
		status = PSA_ERROR_BAD_STATE;
	}

	return status;
}
