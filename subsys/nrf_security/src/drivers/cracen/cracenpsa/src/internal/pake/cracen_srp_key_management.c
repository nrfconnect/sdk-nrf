/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/pake/cracen_srp_key_management.h>

#include <string.h>
#include <silexpk/core.h>
#include <silexpk/ec_curves.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>

extern const uint8_t cracen_N3072[384];

psa_status_t import_srp_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			    size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
			    size_t *key_buffer_length, size_t *key_bits)
{
	size_t bits = psa_get_key_bits(attributes);
	psa_key_type_t type = psa_get_key_type(attributes);

	if (key_buffer_size < data_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (!memcpy_check_non_zero(key_buffer, key_buffer_size, data, data_length)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	switch (type) {
	case PSA_KEY_TYPE_SRP_KEY_PAIR(PSA_DH_FAMILY_RFC3526):
		if (bits != 0 && bits != PSA_BYTES_TO_BITS(sizeof(cracen_N3072))) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
	case PSA_KEY_TYPE_SRP_PUBLIC_KEY(PSA_DH_FAMILY_RFC3526):
		if (bits != 0 && bits != PSA_BYTES_TO_BITS(sizeof(cracen_N3072))) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		if (data_length != sizeof(cracen_N3072)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		if (cracen_be_cmp(key_buffer, cracen_N3072, sizeof(cracen_N3072), 0) >= 0) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		break;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*key_buffer_length = data_length;
	*key_bits = CRACEN_SRP_RFC3526_KEY_BITS_SIZE;

	return PSA_SUCCESS;
}
