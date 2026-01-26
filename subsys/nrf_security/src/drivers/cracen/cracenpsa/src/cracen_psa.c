/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <cracen_psa.h>
#include <cracen/common.h>
#include <cracen/cracen_kmu.h>
#include "cracen_psa_primitives.h"
#include "psa/crypto_sizes.h"
#include "psa/crypto_types.h"
#include "psa/crypto_values.h"

static psa_status_t cracen_get_ikg_opaque_key_size(const psa_key_attributes_t *attributes,
						   size_t *key_size)
{
	switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes))) {
	case CRACEN_BUILTIN_IDENTITY_KEY_ID:
		if (psa_get_key_type(attributes) ==
		    PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)) {
			*key_size = sizeof(ikg_opaque_key);
			return PSA_SUCCESS;
		}
		break;
	case CRACEN_BUILTIN_MEXT_ID:
	case CRACEN_BUILTIN_MKEK_ID:
		if (psa_get_key_type(attributes) == PSA_KEY_TYPE_AES) {
			*key_size = sizeof(ikg_opaque_key);
			return PSA_SUCCESS;
		}
		break;
	}

	return PSA_ERROR_INVALID_ARGUMENT;
}

psa_status_t cracen_get_opaque_size(const psa_key_attributes_t *attributes, size_t *key_size)
{
	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN) {
		return cracen_get_ikg_opaque_key_size(attributes, key_size);
	}

	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN_KMU) {
		if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
			if (psa_get_key_type(attributes) ==
			    PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)) {
				*key_size = PSA_EXPORT_PUBLIC_KEY_OUTPUT_SIZE(
					psa_get_key_type(attributes), psa_get_key_bits(attributes));
			} else {
				*key_size = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
			}
		} else if (psa_get_key_type(attributes) == PSA_KEY_TYPE_HMAC) {
			*key_size = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
		} else {
			*key_size = sizeof(kmu_opaque_key_buffer);
		}

		return PSA_SUCCESS;
	}

	return PSA_ERROR_INVALID_ARGUMENT;
}
