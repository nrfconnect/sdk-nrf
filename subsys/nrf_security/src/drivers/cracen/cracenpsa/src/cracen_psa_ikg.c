/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa_ikg.h>

#include <stdint.h>
#include <string.h>
#include <cracen/statuscodes.h>
#include <cracen_psa_primitives.h>
#include <cracen/common.h>

static void cracen_set_ikg_key_buffer(psa_key_attributes_t *attributes,
				      psa_drv_slot_number_t slot_number, uint8_t *key_buffer)
{
	ikg_opaque_key *ikg_key = (ikg_opaque_key *)key_buffer;

	switch (slot_number) {
	case CRACEN_BUILTIN_IDENTITY_KEY_ID:
		/* The slot_number is not used with the identity key */
		break;
	case CRACEN_BUILTIN_MKEK_ID:
		ikg_key->slot_number = CRACEN_INTERNAL_HW_KEY1_ID;
		break;
	case CRACEN_BUILTIN_MEXT_ID:
		ikg_key->slot_number = CRACEN_INTERNAL_HW_KEY2_ID;
		break;
	}

	ikg_key->owner_id = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(psa_get_key_id(attributes));
}

psa_status_t cracen_ikg_get_builtin_key(psa_drv_slot_number_t slot_number,
					psa_key_attributes_t *attributes,
					uint8_t *key_buffer, size_t key_buffer_size,
					size_t *key_buffer_length)
{
	size_t opaque_key_size;
	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;

	/* According to the PSA Crypto Driver specification, the PSA core will set the `id`
	 * and the `lifetime` field of the attribute struct. We will fill all the other
	 * attributes, and update the `lifetime` field to be more specific.
	 */
	switch (slot_number) {
	case CRACEN_BUILTIN_IDENTITY_KEY_ID:
		psa_set_key_lifetime(attributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
							 CRACEN_KEY_PERSISTENCE_READ_ONLY,
							 PSA_KEY_LOCATION_CRACEN));
		psa_set_key_type(attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(attributes, 256);
		psa_set_key_algorithm(attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
		psa_set_key_usage_flags(attributes, PSA_KEY_USAGE_SIGN_MESSAGE |
							    PSA_KEY_USAGE_SIGN_HASH |
							    PSA_KEY_USAGE_VERIFY_HASH |
							    PSA_KEY_USAGE_VERIFY_MESSAGE);

		status = cracen_get_opaque_size(attributes, &opaque_key_size);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/* According to the PSA Crypto Driver interface proposed document the driver
		 * should fill the attributes even if the buffer of the key is too small. So
		 * we check the buffer here and not earlier in the function.
		 */
		if (key_buffer_size >= opaque_key_size) {
			*key_buffer_length = opaque_key_size;
			cracen_set_ikg_key_buffer(attributes, slot_number, key_buffer);
			return PSA_SUCCESS;
		} else {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}
		break;

	case CRACEN_BUILTIN_MKEK_ID:
	case CRACEN_BUILTIN_MEXT_ID:
		psa_set_key_lifetime(attributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
							 CRACEN_KEY_PERSISTENCE_READ_ONLY,
							 PSA_KEY_LOCATION_CRACEN));
		psa_set_key_type(attributes, PSA_KEY_TYPE_AES);
		psa_set_key_bits(attributes, 256);
		psa_set_key_algorithm(attributes, PSA_ALG_SP800_108_COUNTER_CMAC);
		psa_set_key_usage_flags(attributes,
					PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_VERIFY_DERIVATION);

		status = cracen_get_opaque_size(attributes, &opaque_key_size);
		if (status != PSA_SUCCESS) {
			return status;
		}
		/* See comment about the placement of this check in the previous switch
		 * case.
		 */
		if (key_buffer_size >= opaque_key_size) {
			*key_buffer_length = opaque_key_size;
			cracen_set_ikg_key_buffer(attributes, slot_number, key_buffer);
			return PSA_SUCCESS;
		} else {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}
