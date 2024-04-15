/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa.h>
#include <cracen_psa_kmu.h>
#include <hw_unique_key.h>
#include "hw_unique_key_internal.h"

int hw_unique_key_derive_key(enum hw_unique_key_slot key_slot, const uint8_t *context,
			     size_t context_size, uint8_t const *label, size_t label_size,
			     uint8_t *output, uint32_t output_size)
{
	psa_key_attributes_t mkek_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	if (key_slot == HUK_KEYSLOT_MKEK) {
		psa_set_key_id(&mkek_attr, mbedtls_svc_key_id_make(0, CRACEN_BUILTIN_MKEK_ID));
	} else if (key_slot == HUK_KEYSLOT_MEXT) {
		psa_set_key_id(&mkek_attr, mbedtls_svc_key_id_make(0, CRACEN_BUILTIN_MEXT_ID));
	} else {
		return -HW_UNIQUE_KEY_ERR_MISSING;
	}

	psa_set_key_type(&mkek_attr, PSA_KEY_TYPE_AES);
	psa_set_key_lifetime(&mkek_attr,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN));

	cracen_key_derivation_operation_t op = {};

	status = cracen_key_derivation_setup(&op, PSA_ALG_SP800_108_COUNTER_CMAC);
	if (status != PSA_SUCCESS) {
		if (status == PSA_ERROR_DOES_NOT_EXIST) {
			return -HW_UNIQUE_KEY_ERR_MISSING;
		}
		return -HW_UNIQUE_KEY_ERR_DERIVE_FAILED;
	}
	status = cracen_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET, &mkek_attr,
						 NULL, 0);
	if (status != PSA_SUCCESS) {
		return -HW_UNIQUE_KEY_ERR_DERIVE_FAILED;
	}
	status = cracen_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_LABEL, label,
						   label_size);
	if (status != PSA_SUCCESS) {
		return -HW_UNIQUE_KEY_ERR_DERIVE_FAILED;
	}
	status = cracen_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_CONTEXT, context,
						   context_size);
	if (status != PSA_SUCCESS) {
		return -HW_UNIQUE_KEY_ERR_DERIVE_FAILED;
	}
	status = cracen_key_derivation_output_bytes(&op, output, output_size);
	if (status != PSA_SUCCESS) {
		return -HW_UNIQUE_KEY_ERR_DERIVE_FAILED;
	}

	return HW_UNIQUE_KEY_SUCCESS;
}

bool hw_unique_key_are_any_written(void)
{

	psa_key_lifetime_t lifetime;
	psa_drv_slot_number_t slot_number;
	mbedtls_svc_key_id_t key_id = mbedtls_svc_key_id_make(
		0, PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_SEED, 0));
	return cracen_kmu_get_key_slot(key_id, &lifetime, &slot_number) == PSA_SUCCESS;
}

int hw_unique_key_write(enum hw_unique_key_slot key_slot, const uint8_t *key)
{
	size_t key_bits;
	uint8_t opaque_buffer[2];
	size_t outlen;
	psa_status_t status;
	(void)key_slot;

	psa_key_attributes_t seed_attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_id(&seed_attr,
		       mbedtls_svc_key_id_make(0, PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
							  CRACEN_KMU_KEY_USAGE_SCHEME_SEED, 0)));
	psa_set_key_type(&seed_attr, PSA_KEY_TYPE_RAW_DATA);
	psa_set_key_lifetime(&seed_attr,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_PERSISTENCE_DEFAULT, PSA_KEY_LOCATION_CRACEN_KMU));

	status = cracen_import_key(&seed_attr, key, HUK_SIZE_BYTES, opaque_buffer,
				   sizeof(opaque_buffer), &outlen, &key_bits);

	if (status != PSA_SUCCESS) {
		return -HW_UNIQUE_KEY_ERR_WRITE_FAILED;
	}

	if (key_bits != PSA_BYTES_TO_BITS(HUK_SIZE_BYTES)) {
		return -HW_UNIQUE_KEY_ERR_GENERIC_ERROR;
	}

	return HW_UNIQUE_KEY_SUCCESS;
}
int hw_unique_key_write_random(void)
{
	psa_status_t status;
	uint8_t random_data[HUK_SIZE_BYTES];

	status = cracen_get_random(NULL, random_data, sizeof(random_data));
	if (status != PSA_SUCCESS) {
		return -HW_UNIQUE_KEY_ERR_GENERATION_FAILED;
	}

	return hw_unique_key_write(0, random_data);
}

bool hw_unique_key_is_written(enum hw_unique_key_slot key_slot)
{
	return hw_unique_key_are_any_written();
}
