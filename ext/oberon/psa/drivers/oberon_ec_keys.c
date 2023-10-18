/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include "psa/crypto.h"
#include "oberon_ec_keys.h"
#include "oberon_helpers.h"

#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_224
#include "ocrypto_ecdh_p224.h"
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_256
#include "ocrypto_ecdh_p256.h"
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_384
#include "ocrypto_ecdh_p384.h"
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_384 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_MONTGOMERY_255
#include "ocrypto_curve25519.h"
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_MONTGOMERY_255 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_TWISTED_EDWARDS_255
#include "ocrypto_ed25519.h"
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_TWISTED_EDWARDS_255 */

psa_status_t oberon_export_ec_public_key(const psa_key_attributes_t *attributes, const uint8_t *key,
					 size_t key_length, uint8_t *data, size_t data_size,
					 size_t *data_length)
{
	int res;
	size_t bits = psa_get_key_bits(attributes);
	psa_key_type_t type = psa_get_key_type(attributes);

	if (PSA_KEY_TYPE_IS_PUBLIC_KEY(type)) {
		if (key_length > data_size) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}
		memcpy(data, key, key_length);
		*data_length = key_length;
		return PSA_SUCCESS;
	}

	switch (type) {
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP
	case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
		if (data_size < key_length * 2 + 1) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}
		*data_length = key_length * 2 + 1;
		data[0] = 0x04;
		switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_224
		case 224:
			res = ocrypto_ecdh_p224_public_key(&data[1], key);
			break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_256
		case 256:
			res = ocrypto_ecdh_p256_public_key(&data[1], key);
			break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_384
		case 384:
			res = ocrypto_ecdh_p384_public_key(&data[1], key);
			break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_384 */
		default:
			return PSA_ERROR_NOT_SUPPORTED;
		}
		if (res) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_MONTGOMERY_255
	case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY):
		if (bits != 255) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		if (data_size < key_length) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}
		*data_length = key_length;
		ocrypto_curve25519_scalarmult_base(data, key);
		break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_MONTGOMERY_255 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_TWISTED_EDWARDS_255
	case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS):
		if (bits != 255) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		if (data_size < key_length) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}
		*data_length = key_length;
		ocrypto_ed25519_public_key(data, key);
		break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_TWISTED_EDWARDS_255 */
	default:
		(void)res;
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return PSA_SUCCESS;
}

psa_status_t oberon_import_ec_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				  size_t data_length, uint8_t *key, size_t key_size,
				  size_t *key_length, size_t *key_bits)
{
	int res;
	size_t bits = psa_get_key_bits(attributes);
	psa_key_type_t type = psa_get_key_type(attributes);

	switch (type) {
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP
	case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
		if (bits == 0) {
			bits = PSA_BYTES_TO_BITS(data_length);
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_P521
			if (bits == 528) {
				bits = 521;
			}
#endif
		}
		switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_224
		case 224:
			if (data_length != 28) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			if (!oberon_ct_compare_zero(data, 28)) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			res = ocrypto_ecdh_p224_secret_key_check(data);
			break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_256
		case 256:
			if (data_length != 32) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			if (!oberon_ct_compare_zero(data, 32)) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			res = ocrypto_ecdh_p256_secret_key_check(data);
			break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_384
		case 384:
			if (data_length != 48) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			if (!oberon_ct_compare_zero(data, 48)) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			res = ocrypto_ecdh_p384_secret_key_check(data);
			break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_384 */
		default:
			return PSA_ERROR_NOT_SUPPORTED;
		}
		if (res) {
			return PSA_ERROR_INVALID_ARGUMENT; // out of range
		}
		break;
	case PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1):
		if (bits == 0) {
			if ((data_length & 1) == 0) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			bits = PSA_BYTES_TO_BITS(data_length >> 1);
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_P521
			if (bits == 528) {
				bits = 521;
			}
#endif
		}
		switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_224
		case 224:
			if (data_length != 57 || data[0] != 0x04) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			res = ocrypto_ecdh_p224_public_key_check(&data[1]);
			break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_256
		case 256:
			if (data_length != 65 || data[0] != 0x04) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			res = ocrypto_ecdh_p256_public_key_check(&data[1]);
			break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_384
		case 384:
			if (data_length != 97 || data[0] != 0x04) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			res = ocrypto_ecdh_p384_public_key_check(&data[1]);
			break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_384 */
		default:
			return PSA_ERROR_NOT_SUPPORTED;
		}
		if (res) {
			return PSA_ERROR_INVALID_ARGUMENT; // point not on curve
		}
		break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP */
#if defined(PSA_NEED_OBERON_KEY_MANAGEMENT_MONTGOMERY) ||                                          \
	defined(PSA_NEED_OBERON_KEY_MANAGEMENT_TWISTED_EDWARDS)
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_MONTGOMERY_255
	case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY):
	case PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_MONTGOMERY):
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_MONTGOMERY_255 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_TWISTED_EDWARDS_255
	case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS):
	case PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS):
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_TWISTED_EDWARDS_255 */
		if (bits == 0) {
			if (data_length == 32) {
				bits = 255;
			} else {
				return PSA_ERROR_NOT_SUPPORTED;
			}
		}
		if (data_length != PSA_BITS_TO_BYTES(bits)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		switch (bits) {
		case 255:
			break;
		default:
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_MONTGOMERY ||                                             \
	  PSA_NEED_OBERON_KEY_MANAGEMENT_TWISTED_EDWARDS */
	default:
		(void)res;
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (*key_bits != 0 && *key_bits != bits) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	if (key_size < data_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}
	memcpy(key, data, data_length);
	if (type == PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY)) {
		// enforce constant bits
		key[0] = (uint8_t)(key[0] & 0xF8);
		key[31] = (uint8_t)((key[31] & 0x7F) | 0x40);
	}
	*key_length = data_length;
	*key_bits = bits;
	return PSA_SUCCESS;
}

psa_status_t oberon_generate_ec_key(const psa_key_attributes_t *attributes, uint8_t *key,
				    size_t key_size, size_t *key_length)
{
	int res;
	psa_status_t status;
	size_t bits = psa_get_key_bits(attributes);
	psa_key_type_t type = psa_get_key_type(attributes);
	size_t length = PSA_BITS_TO_BYTES(bits);

	if (key_size < length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}
	*key_length = length;

	switch (type) {
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP
	case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
		do {
			status = psa_generate_random(key, length);
			if (status) {
				return status;
			}
			if (!oberon_ct_compare_zero(key, length)) {
				continue;
			}
			switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_224
			case 224:
				res = ocrypto_ecdh_p224_secret_key_check(key);
				break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_256
			case 256:
				res = ocrypto_ecdh_p256_secret_key_check(key);
				break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_384
			case 384:
				res = ocrypto_ecdh_p384_secret_key_check(key);
				break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP_R1_384 */
			default:
				return PSA_ERROR_NOT_SUPPORTED;
			}
		} while (res);
		break;
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_SECP */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_MONTGOMERY_255
	case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY):
		if (bits != 255) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		return psa_generate_random(key, length);
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_MONTGOMERY_255 */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_TWISTED_EDWARDS_255
	case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS):
		if (bits != 255) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		return psa_generate_random(key, length);
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_TWISTED_EDWARDS_255 */
	default:
		(void)key;
		(void)res;
		(void)status;
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return PSA_SUCCESS;
}
