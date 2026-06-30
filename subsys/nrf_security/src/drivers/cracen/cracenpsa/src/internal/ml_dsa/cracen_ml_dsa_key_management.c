/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/ml_dsa/cracen_ml_dsa_key_management.h>
#include "cracen_ml_dsa_internal.h"

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <string.h>
#include <zephyr/sys/util.h>

/* All possible PSA key-bits values of the supported parameter sets according to FIPS 204, Table 1:
 * 128 for ML-DSA-44,
 * 192 for ML-DSA-65,
 * 256 for ML-DSA-87.
 */
static const size_t ml_dsa_supported_bits[] = {
	IF_ENABLED(CONFIG_PSA_NEED_CRACEN_ML_DSA_44, (128)),
	IF_ENABLED(CONFIG_PSA_NEED_CRACEN_ML_DSA_65, (192)),
	IF_ENABLED(CONFIG_PSA_NEED_CRACEN_ML_DSA_87, (256)),
};

/* Validate the attributes of an ML-DSA public key and resolve the key bits.
 * When @p key_bits is 0 on entry it is derived from @p key_size_bytes; otherwise
 * it must match a supported parameter set whose public-key size equals
 * @p key_size_bytes.
 */
static psa_status_t check_ml_dsa_key_attributes(const psa_key_attributes_t *attributes,
						size_t key_size_bytes, size_t *key_bits)
{
	psa_algorithm_t alg = psa_get_key_algorithm(attributes);
	bool is_public_key = PSA_KEY_TYPE_IS_PUBLIC_KEY(psa_get_key_type(attributes));
	size_t valid_key_size;

	if (!PSA_ALG_IS_ML_DSA(alg) && !PSA_ALG_IS_HASH_ML_DSA(alg)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	for (uint32_t i = 0; i < ARRAY_SIZE(ml_dsa_supported_bits); i++) {
		const ml_dsa_params_t *p = cracen_ml_dsa_params_get(ml_dsa_supported_bits[i]);

		if (p == NULL) {
			continue;
		}
		valid_key_size = is_public_key ? p->pk_size : p->priv_key_size;

		if (*key_bits == 0 && valid_key_size == key_size_bytes) {
			*key_bits = ml_dsa_supported_bits[i];
		}

		if (*key_bits == ml_dsa_supported_bits[i]) {
			if (valid_key_size != key_size_bytes) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			return PSA_SUCCESS;
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t import_ml_dsa_public_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				      size_t data_length, uint8_t *key_buffer,
				      size_t key_buffer_size, size_t *key_buffer_length,
				      size_t *key_bits)
{
	size_t bits = psa_get_key_bits(attributes);
	psa_status_t status;

	if (data_length > key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	status = check_ml_dsa_key_attributes(attributes, data_length, &bits);
	if (status != PSA_SUCCESS) {
		return status;
	}

	memcpy(key_buffer, data, data_length);
	*key_bits = bits;
	*key_buffer_length = data_length;

	return PSA_SUCCESS;
}

psa_status_t export_ml_dsa_public_key(const uint8_t *key_buffer, size_t key_buffer_size,
				      uint8_t *data, size_t data_size, size_t *data_length)
{
	if (data_size < key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(data, key_buffer, key_buffer_size);
	*data_length = key_buffer_size;
	return PSA_SUCCESS;
}
