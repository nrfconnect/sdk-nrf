/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"
#include <cracen/ec_helpers.h>
#include <cracen/mem_helpers.h>
#include "cracen_psa.h"
#include "platform_keys/platform_keys.h"
#include <nrf_security_mutexes.h>

#include <sicrypto/drbghash.h>
#include <sicrypto/ecc.h>
#include <sicrypto/ecdsa.h>
#include <sicrypto/ed25519.h>
#include <sicrypto/ed448.h>
#include <sicrypto/montgomery.h>
#include <sicrypto/rsa_keygen.h>
#include <sicrypto/util.h>
#include <silexpk/ed448.h>
#include <silexpk/sxops/rsa.h>
#include <sicrypto/ik.h>
#include <silexpk/ik.h>
#include <stddef.h>
#include <string.h>
#include <sxsymcrypt/trng.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

extern const uint8_t cracen_N3072[384];

extern nrf_security_mutex_t cracen_mutex_symmetric;

#define DEFAULT_KEY_SIZE(bits) (bits), PSA_BITS_TO_BYTES(bits), (1 + 2 * PSA_BITS_TO_BYTES(bits))
static struct {
	psa_ecc_family_t family;
	size_t bits;
	size_t private_key_size_bytes;
	size_t public_key_size_bytes;
	bool supported;
} valid_keys[] = {
	/* Brainpool */
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(192),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(224),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(256),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(320),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(384),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(512),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},

	/* SECP k1 */
	{PSA_ECC_FAMILY_SECP_K1, DEFAULT_KEY_SIZE(192),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_K1)},
	{PSA_ECC_FAMILY_SECP_K1, DEFAULT_KEY_SIZE(225), false}, /* NCSDK-21311 */
	{PSA_ECC_FAMILY_SECP_K1, DEFAULT_KEY_SIZE(256),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_K1)},

	/* SECP r1 */
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(192),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(224),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(256),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(320),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(384),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(521),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},

	/* Montgomery */
	{PSA_ECC_FAMILY_MONTGOMERY, 255, 32, 32,
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_MONTGOMERY)},
	{PSA_ECC_FAMILY_MONTGOMERY, 448, 56, 56,
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_MONTGOMERY)},

	/* Twisted Edwards */
	{PSA_ECC_FAMILY_TWISTED_EDWARDS, 255, 32, 32,
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_TWISTED_EDWARDS)},
	{PSA_ECC_FAMILY_TWISTED_EDWARDS, 448, 57, 57, false},
};

static psa_status_t check_ecc_key_attributes(const psa_key_attributes_t *attributes,
					     size_t key_buffer_size, size_t *key_bits)
{
	psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes));
	psa_algorithm_t key_alg = psa_get_key_algorithm(attributes);
	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	bool is_public_key = PSA_KEY_TYPE_IS_PUBLIC_KEY(psa_get_key_type(attributes));

	for (size_t i = 0; i < ARRAY_SIZE(valid_keys); i++) {
		if (valid_keys[i].family == curve) {
			size_t valid_key_size = is_public_key
							? valid_keys[i].public_key_size_bytes
							: valid_keys[i].private_key_size_bytes;

			if (*key_bits == 0 && valid_key_size == key_buffer_size) {
				*key_bits = valid_keys[i].bits;

				status = valid_keys[i].supported ? PSA_SUCCESS
								 : PSA_ERROR_NOT_SUPPORTED;
				break;
			}

			if (*key_bits == valid_keys[i].bits) {
				if (valid_key_size != key_buffer_size) {
					return PSA_ERROR_INVALID_ARGUMENT;
				}

				status = valid_keys[i].supported ? PSA_SUCCESS
								 : PSA_ERROR_NOT_SUPPORTED;
				break;
			}
		}
	}

	if (status == PSA_SUCCESS) {
		if (curve == PSA_ECC_FAMILY_TWISTED_EDWARDS) {
			if (key_alg != PSA_ALG_PURE_EDDSA && key_alg != PSA_ALG_ED25519PH) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
		}
	}

	return status;
}

static psa_status_t check_rsa_key_attributes(const psa_key_attributes_t *attributes,
					     size_t key_bits)
{
	psa_algorithm_t key_alg = psa_get_key_algorithm(attributes);

	if (!PSA_ALG_IS_RSA_PKCS1V15_SIGN(key_alg) && !PSA_ALG_IS_RSA_PSS(key_alg) &&
	    !PSA_ALG_IS_RSA_OAEP(key_alg) && (key_alg != PSA_ALG_RSA_PKCS1V15_CRYPT)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	switch (key_bits) {
	case 1024:
		return PSA_SUCCESS;
	case 2048:
		return PSA_SUCCESS;
	case 3072:
		return PSA_SUCCESS;
	case 4096:
		return PSA_SUCCESS;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

static size_t get_asn1_size_with_tag_and_length(size_t sz)
{
	size_t r = 2 + sz; /* 1 byte tag, 1 byte for size and buffer size. */

	/* If size >= 0x80 we need additional bytes. */
	if (sz >= 0x80) {
		while (sz) {
			r += 1;
			sz >>= 8;
		}
	}

	return r;
}

static void write_tag_and_length(struct sx_buf *buf, uint8_t tag)
{
	size_t length = buf->sz;
	uint8_t *outbuf = buf->bytes - get_asn1_size_with_tag_and_length(buf->sz) + length;

	*outbuf++ = tag;
	if (length < 0x80) {
		*outbuf = length;
		return;
	}

	uint8_t len_bytes = get_asn1_size_with_tag_and_length(buf->sz) - length - 2;
	*outbuf++ = 0x80 | len_bytes;

	/* Write out length as big endian. */
	length = sys_cpu_to_be32(length);
	memcpy(outbuf, ((uint8_t *)&length) + sizeof(length) - len_bytes, len_bytes);
}

static size_t calc_key_bits_from_pub_key_buffer_size(psa_ecc_family_t curve, size_t pub_key_size)
{
	switch (curve) {
	case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
	case PSA_ECC_FAMILY_SECP_R1: {
		size_t priv_key_size = (pub_key_size - 1) / 2;

		if (priv_key_size == PSA_BITS_TO_BYTES(521)) {
			/* The secpr1p521 is a special case since the number of
			 * bits are not divisible by 8
			 */
			return 521;
		}
		return PSA_BYTES_TO_BITS(priv_key_size);
	}
	case PSA_ECC_FAMILY_MONTGOMERY:
		if (pub_key_size == PSA_BITS_TO_BYTES(255)) {
			return 255;
		}
		return PSA_BYTES_TO_BITS(pub_key_size);
	case PSA_ECC_FAMILY_TWISTED_EDWARDS:
		if (pub_key_size == PSA_BITS_TO_BYTES(255)) {
			return 255;
		}
		return 0;
	default:
		return 0;
	}
}

static psa_status_t import_ecc_private_key(const psa_key_attributes_t *attributes,
					   const uint8_t *data, size_t data_length,
					   uint8_t *key_buffer, size_t key_buffer_size,
					   size_t *key_buffer_length, size_t *key_bits)
{
	size_t key_bits_attr = psa_get_key_bits(attributes);
	psa_status_t psa_status;

	if (data_length > key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	psa_status = check_ecc_key_attributes(attributes, data_length, &key_bits_attr);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	/* TODO: NCSDK-24516: Here we don't check if key < n.
	 * We don't consider this a security issue since it should return an
	 * error when tried to be used. Remove the comment when more testing is
	 * done and we can verify that this is not needed.
	 */
	if (memcpy_check_non_zero(key_buffer, key_buffer_size, data, data_length)) {
		*key_bits = key_bits_attr;
		*key_buffer_length = data_length;
		return PSA_SUCCESS;
	} else {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

static psa_status_t check_wstr_publ_key_for_ecdh(psa_ecc_family_t curve_family, size_t curve_bits,
						 const uint8_t *data, size_t data_length)
{
	size_t priv_key_size = PSA_BITS_TO_BYTES(curve_bits);
	psa_status_t psa_status;
	const struct sx_pk_ecurve *curve;

	sx_pk_affine_point publ_key_pnt = {};

	publ_key_pnt.x.bytes = (uint8_t *)&data[1];
	publ_key_pnt.x.sz = priv_key_size;

	publ_key_pnt.y.bytes = (uint8_t *)&data[1 + priv_key_size];
	publ_key_pnt.y.sz = priv_key_size;

	psa_status = cracen_ecc_get_ecurve_from_psa(curve_family, curve_bits, &curve);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	return cracen_ecc_check_public_key(curve, &publ_key_pnt);
}

static psa_status_t check_wstr_pub_key_data(psa_algorithm_t key_alg, psa_ecc_family_t curve,
					    size_t key_bits, const uint8_t *data,
					    size_t data_length)
{
	size_t expected_pub_key_size =
		cracen_ecc_wstr_expected_pub_key_bytes(PSA_BITS_TO_BYTES(key_bits));

	if (data[0] != SI_ECC_PUBKEY_UNCOMPRESSED) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_length != expected_pub_key_size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (PSA_ALG_IS_ECDSA(key_alg)) {
		return PSA_SUCCESS;
	} else if (PSA_ALG_IS_ECDH(key_alg)) {
		return check_wstr_publ_key_for_ecdh(curve, key_bits, data, data_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

static psa_status_t import_ecc_public_key(const psa_key_attributes_t *attributes,
					  const uint8_t *data, size_t data_length,
					  uint8_t *key_buffer, size_t key_buffer_size,
					  size_t *key_buffer_length, size_t *key_bits)
{
	size_t key_bits_attr = psa_get_key_bits(attributes);
	psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes));
	psa_algorithm_t key_alg = psa_get_key_algorithm(attributes);
	psa_status_t psa_status;

	uint8_t local_key_buffer[PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(
		PSA_VENDOR_ECC_MAX_CURVE_BITS)];

	if (!memcpy_check_non_zero(local_key_buffer, sizeof(local_key_buffer), data, data_length)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_length > key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	psa_status = check_ecc_key_attributes(attributes, data_length, &key_bits_attr);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	switch (curve) {
	case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
	case PSA_ECC_FAMILY_SECP_R1:
		psa_status = check_wstr_pub_key_data(key_alg, curve, key_bits_attr,
						     local_key_buffer, data_length);
		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}
		break;
	/* prevent -Wswitch warning*/
	default:
		break;
	}

	memcpy(key_buffer, local_key_buffer, data_length);
	*key_bits = key_bits_attr;
	*key_buffer_length = data_length;

	return PSA_SUCCESS;
}

static psa_status_t import_rsa_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				   size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
				   size_t *key_buffer_length, size_t *key_bits)
{
	size_t key_bits_attr = psa_get_key_bits(attributes);
	psa_key_type_t key_type = psa_get_key_type(attributes);
	bool is_public_key = key_type == PSA_KEY_TYPE_RSA_PUBLIC_KEY;

	struct si_rsa_key rsakey;
	struct sx_buf n = {0};
	struct sx_buf e = {0};

	if (data_length > key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* We copy the key to the internal buffer first, then validate it. */
	memcpy(key_buffer, data, data_length);

	psa_status_t status = silex_statuscodes_to_psa(cracen_signature_get_rsa_key(
		&rsakey, is_public_key, key_type == PSA_KEY_TYPE_RSA_KEY_PAIR, key_buffer,
		data_length, &n, &e));

	if (status != PSA_SUCCESS) {
		goto cleanup;
	}

	/* When importing keys the PSA APIs allow for key bits to be 0 and they
	 * expect it to be calculated based on the buffer size of the data. For
	 * RSA keys the key size is the size of the modulus.
	 */
	if (key_bits_attr == 0) {
		key_bits_attr = PSA_BYTES_TO_BITS(n.sz);
	}

	status = check_rsa_key_attributes(attributes, key_bits_attr);
	if (status != PSA_SUCCESS) {
		goto cleanup;
	}

	*key_buffer_length = data_length;
	*key_bits = key_bits_attr;

	return PSA_SUCCESS;

cleanup:
	*key_buffer_length = 0;
	*key_bits = 0;
	safe_memzero(key_buffer, key_buffer_size);
	return status;
}

static psa_status_t import_spake2p_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				       size_t data_length, uint8_t *key_buffer,
				       size_t key_buffer_size, size_t *key_buffer_length,
				       size_t *key_bits)
{
	size_t bits = psa_get_key_bits(attributes);
	psa_key_type_t type = psa_get_key_type(attributes);

	/* Check for invalid key bits*/
	if (bits != 0 && (bits != 256)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (key_buffer_size < data_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* We only support 256 bit keys and they PSA APIs does not enforce setting the key bits. */
	bits = 256;

	switch (type) {
	case PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
		/* These keys contains w0 and w1. */
		if (data_length != CRACEN_P256_KEY_SIZE * 2) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		/* Do not allow w0 to be 0. */
		if (!memcpy_check_non_zero(key_buffer, key_buffer_size, data,
					   CRACEN_P256_KEY_SIZE)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		/* Do not allow w1 to be 0. */
		if (!memcpy_check_non_zero(key_buffer + CRACEN_P256_KEY_SIZE,
					   key_buffer_size - CRACEN_P256_KEY_SIZE,
					   data + CRACEN_P256_KEY_SIZE, CRACEN_P256_KEY_SIZE)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		/* We don't check if the keys are in range. This will generate an error on usage. */
		break;
	case PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1):
		/* These keys contains w0 and L. L is an uncompressed point. */
		if (data_length != CRACEN_P256_KEY_SIZE + CRACEN_P256_POINT_SIZE + 1) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		/* Do not allow w0 to be 0. */
		if (!memcpy_check_non_zero(key_buffer, key_buffer_size, data,
					   CRACEN_P256_KEY_SIZE)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		/* Validate L */
		uint8_t L[CRACEN_P256_POINT_SIZE + 1];

		memcpy(L, &data[CRACEN_P256_KEY_SIZE], sizeof(L));
		if (check_wstr_pub_key_data(PSA_ALG_ECDH, PSA_ECC_FAMILY_SECP_R1, bits, L,
					    sizeof(L))) {
			safe_memzero(L, sizeof(L));
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		memcpy(key_buffer + CRACEN_P256_KEY_SIZE, L, sizeof(L));

		break;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*key_buffer_length = data_length;
	*key_bits = bits;

	return PSA_SUCCESS;
}

static psa_status_t import_srp_key(const psa_key_attributes_t *attributes, const uint8_t *data,
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
		if (si_be_cmp(key_buffer, cracen_N3072, sizeof(cracen_N3072), 0) >= 0) {
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

static psa_status_t generate_ecc_private_key(const psa_key_attributes_t *attributes,
					     uint8_t *key_buffer, size_t key_buffer_size,
					     size_t *key_buffer_length)
{
#if PSA_VENDOR_ECC_MAX_CURVE_BITS > 0
	size_t key_bits_attr = psa_get_key_bits(attributes);
	size_t key_size_bytes = PSA_BITS_TO_BYTES(key_bits_attr);
	psa_ecc_family_t psa_curve = PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes));
	psa_status_t psa_status;
	int si_status;
	struct sitask t;
	const struct sx_pk_ecurve *sx_curve;
	struct si_eccsk si_priv_key;
	uint8_t workmem[PSA_KEY_EXPORT_ECC_KEY_PAIR_MAX_SIZE(PSA_VENDOR_ECC_MAX_CURVE_BITS)] = {};

	*key_buffer_length = 0;

	if (key_size_bytes > key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (key_size_bytes > sizeof(workmem)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	psa_status = check_ecc_key_attributes(attributes, key_buffer_size, &key_bits_attr);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	psa_status = cracen_ecc_get_ecurve_from_psa(psa_curve, key_bits_attr, &sx_curve);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	if (psa_curve == PSA_ECC_FAMILY_MONTGOMERY || psa_curve == PSA_ECC_FAMILY_TWISTED_EDWARDS) {
		psa_status = cracen_get_random(NULL, workmem, key_size_bytes);
		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}

		if (constant_memcmp_is_zero(workmem, key_size_bytes)) {
			return PSA_ERROR_INSUFFICIENT_ENTROPY;
		}

		if (psa_curve == PSA_ECC_FAMILY_MONTGOMERY) {
			/* For X448 and X25519 we must clamp the private keys.
			 */
			if (key_size_bytes == 32) {
				/* X25519 */
				decode_scalar_25519(&workmem[0]);
			} else if (key_size_bytes == 56) {
				/* X448 */
				decode_scalar_448(&workmem[0]);
			}
		}

		memcpy(key_buffer, workmem, key_size_bytes);
	} else {
		si_task_init(&t, workmem, key_size_bytes);

		si_priv_key.d = key_buffer;
		si_ecc_create_genprivkey(&t, sx_curve, &si_priv_key);
		si_task_run(&t);

		si_status = si_task_wait(&t);
		if (si_status != SX_OK) {
			psa_status = silex_statuscodes_to_psa(si_status);
		}
	}

	safe_memzero(workmem, sizeof(workmem));
	*key_buffer_length = key_size_bytes;

	return psa_status;
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_VENDOR_ECC_MAX_CURVE_BITS > 0 */
}

static psa_status_t export_ecc_public_key_from_keypair(const psa_key_attributes_t *attributes,
						       const uint8_t *key_buffer,
						       size_t key_buffer_size, uint8_t *data,
						       size_t data_size, size_t *data_length)
{
	size_t key_bits_attr = psa_get_key_bits(attributes);
	psa_ecc_family_t psa_curve = PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes));
	psa_status_t psa_status;
	size_t expected_pub_key_size = 0;
	int si_status = 0;
	const struct sx_pk_ecurve *sx_curve;
	struct sitask t;

	switch (psa_curve) {
	case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
	case PSA_ECC_FAMILY_SECP_R1:
	case PSA_ECC_FAMILY_SECP_K1:
		expected_pub_key_size =
			cracen_ecc_wstr_expected_pub_key_bytes(PSA_BITS_TO_BYTES(key_bits_attr));
		break;
	case PSA_ECC_FAMILY_MONTGOMERY:
	case PSA_ECC_FAMILY_TWISTED_EDWARDS:
		expected_pub_key_size = PSA_BITS_TO_BYTES(key_bits_attr);
		break;
	}

	if (expected_pub_key_size > data_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	psa_status = cracen_ecc_get_ecurve_from_psa(psa_curve, key_bits_attr, &sx_curve);
	if (psa_status != PSA_SUCCESS) {
		return PSA_SUCCESS;
	}

	struct si_sig_privkey priv_key;
	struct si_sig_pubkey pub_key;

	char workmem[SX_ED448_DGST_SZ] = {};

	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN) {
		if (key_buffer_size != sizeof(ikg_opaque_key)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		priv_key =
			si_sig_fetch_ikprivkey(sx_curve, ((ikg_opaque_key *)key_buffer)->owner_id);
		data[0] = SI_ECC_PUBKEY_UNCOMPRESSED;
		pub_key.key.eckey.qx = &data[1];
		pub_key.key.eckey.qy = &data[1 + sx_pk_curve_opsize(sx_curve)];
	} else {

		switch (psa_curve) {
		case PSA_ECC_FAMILY_SECP_R1:
		case PSA_ECC_FAMILY_SECP_K1:
		case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
			priv_key.def = si_sig_def_ecdsa;
			priv_key.key.eckey.curve = sx_curve;
			priv_key.key.eckey.d = (char *)key_buffer;

			data[0] = SI_ECC_PUBKEY_UNCOMPRESSED;
			pub_key.key.eckey.qx = &data[1];
			pub_key.key.eckey.qy = &data[1 + sx_pk_curve_opsize(sx_curve)];
			break;
		case PSA_ECC_FAMILY_MONTGOMERY:
			if (key_bits_attr == 255) {
				priv_key.def = si_sig_def_x25519;
				priv_key.key.x25519 = (struct sx_x25519_op *)key_buffer;
				pub_key.key.x25519 = (struct sx_x25519_pt *)data;
			} else {
				priv_key.def = si_sig_def_x448;
				priv_key.key.x448 = (struct sx_x448_op *)key_buffer;
				pub_key.key.x448 = (struct sx_x448_pt *)data;
			}
			break;
		case PSA_ECC_FAMILY_TWISTED_EDWARDS:
			if (key_bits_attr == 255) {
				priv_key.def = si_sig_def_ed25519;
				priv_key.key.ed25519 = (struct sx_ed25519_v *)key_buffer;
				pub_key.key.ed25519 = (struct sx_ed25519_pt *)data;
			} else {
				priv_key.def = si_sig_def_ed448;
				priv_key.key.ed448 = (struct sx_ed448_v *)key_buffer;
				pub_key.key.ed448 = (struct sx_ed448_pt *)data;
			}
			break;
		default:
			return PSA_ERROR_NOT_SUPPORTED;
		}
	}

	si_task_init(&t, workmem, sizeof(workmem));
	si_sig_create_pubkey(&t, &priv_key, &pub_key);
	si_task_run(&t);

	si_status = si_task_wait(&t);
	safe_memzero(workmem, sizeof(workmem));
	if (si_status != SX_OK) {
		return silex_statuscodes_to_psa(si_status);
	}

	*data_length = expected_pub_key_size;
	return PSA_SUCCESS;
}
static psa_status_t export_rsa_public_key_from_keypair(const psa_key_attributes_t *attributes,
						       const uint8_t *key_buffer,
						       size_t key_buffer_size, uint8_t *data,
						       size_t data_size, size_t *data_length)
{
	/*
	 * RSAPublicKey ::= SEQUENCE {
	 * modulus            INTEGER,    -- n
	 * publicExponent     INTEGER  }  -- e
	 */

	size_t key_bits_attr = psa_get_key_bits(attributes);
	struct si_rsa_key rsa_key;
	struct sx_buf n = {0};
	struct sx_buf e = {0};

	psa_status_t status = check_rsa_key_attributes(attributes, key_bits_attr);

	if (status != PSA_SUCCESS) {
		return status;
	}

	status = silex_statuscodes_to_psa(cracen_signature_get_rsa_key(
		&rsa_key, true, true, key_buffer, key_buffer_size, &n, &e));

	if (status != PSA_SUCCESS) {
		return status;
	}

	struct sx_buf sequence = {0};

	/* Array of buffers in the order they will be serialized. */
	struct sx_buf *buffers[] = {&n, &e};

	for (size_t i = 0; i < ARRAY_SIZE(buffers); i++) {
		if (buffers[i]->bytes[0] & 0x80) {
			buffers[i]->sz++;
		}
		sequence.sz += get_asn1_size_with_tag_and_length(buffers[i]->sz);
	}

	size_t total_size = get_asn1_size_with_tag_and_length(sequence.sz);

	sequence.bytes = data + total_size - sequence.sz;

	if (total_size > data_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* Start placing buffers from the end */
	size_t offset = total_size;

	for (int i = ARRAY_SIZE(buffers) - 1; i >= 0; i--) {
		uint8_t *new_location = data + offset - buffers[i]->sz;

		if (buffers[i]->bytes[0] & 0x80) {
			*new_location = 0x0;
			memcpy(new_location + 1, buffers[i]->bytes, buffers[i]->sz - 1);
		} else {
			memcpy(new_location, buffers[i]->bytes, buffers[i]->sz);
		}
		buffers[i]->bytes = new_location;
		offset -= get_asn1_size_with_tag_and_length(buffers[i]->sz);
		write_tag_and_length(buffers[i], ASN1_INTEGER);
	}

	write_tag_and_length(&sequence, ASN1_SEQUENCE | ASN1_CONSTRUCTED);

	*data_length = total_size;
	return PSA_SUCCESS;
}

static psa_status_t ecc_export_key(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size, uint8_t *data,
				   size_t data_size, size_t *data_length)
{
	if (data_size < key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(data, key_buffer, key_buffer_size);
	*data_length = key_buffer_size;
	return PSA_SUCCESS;
}

static psa_status_t rsa_export_public_key(const psa_key_attributes_t *attributes,
					  const uint8_t *key_buffer, size_t key_buffer_size,
					  uint8_t *data, size_t data_size, size_t *data_length)
{
	if (data_size < key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (data == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_status_t status = check_rsa_key_attributes(attributes, psa_get_key_bits(attributes));

	if (status != PSA_SUCCESS) {
		return status;
	}

	memcpy(data, key_buffer, key_buffer_size);
	*data_length = key_buffer_size;

	return PSA_SUCCESS;
}

psa_status_t cracen_export_public_key(const psa_key_attributes_t *attributes,
				      const uint8_t *key_buffer, size_t key_buffer_size,
				      uint8_t *data, size_t data_size, size_t *data_length)
{
	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(data_length);

	psa_key_type_t key_type = psa_get_key_type(attributes);
	*data_length = 0;

	if (data_size == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT)) {
		if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type)) {
			return export_ecc_public_key_from_keypair(attributes, key_buffer,
								  key_buffer_size, data, data_size,
								  data_length);
		} else if (PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(key_type)) {
			return ecc_export_key(attributes, key_buffer, key_buffer_size, data,
					      data_size, data_length);
		}
	}

	if (key_type == PSA_KEY_TYPE_RSA_KEY_PAIR &&
	    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_RSA_KEY_PAIR_EXPORT)) {
		return export_rsa_public_key_from_keypair(attributes, key_buffer, key_buffer_size,
							  data, data_size, data_length);
	} else if (key_type == PSA_KEY_TYPE_RSA_PUBLIC_KEY &&
		   IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_RSA_PUBLIC_KEY)) {
		return rsa_export_public_key(attributes, key_buffer, key_buffer_size, data,
					     data_size, data_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			       size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
			       size_t *key_buffer_length, size_t *key_bits)
{
	__ASSERT_NO_MSG(key_buffer);
	__ASSERT_NO_MSG(key_buffer_length);
	__ASSERT_NO_MSG(key_bits);

	psa_key_type_t key_type = psa_get_key_type(attributes);
	*key_bits = 0;
	*key_buffer_length = 0;

	if (key_buffer_size == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));
#ifdef CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
	if (location == PSA_KEY_LOCATION_CRACEN_KMU) {
		int slot_id = CRACEN_PSA_GET_KMU_SLOT(
			MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)));
		psa_key_attributes_t stored_attributes;

		if (key_buffer_size < cracen_get_opaque_size(attributes)) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		psa_status_t status = cracen_kmu_provision(attributes, slot_id, data, data_length);

		if (status != PSA_SUCCESS) {
			return status;
		}

		status = cracen_kmu_get_builtin_key(slot_id, &stored_attributes, key_buffer,
						    key_buffer_size, key_buffer_length);
		if (status != PSA_SUCCESS) {
			return status;
		}

		*key_bits = psa_get_key_bits(&stored_attributes);

		return status;
	}
#endif
#ifdef CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS
	if (location == PSA_KEY_LOCATION_CRACEN) {
		psa_key_lifetime_t lifetime;
		psa_drv_slot_number_t slot_id;
		psa_key_attributes_t stored_attributes;
		psa_status_t status = cracen_platform_keys_provision(attributes, data, data_length);

		if (status != PSA_SUCCESS) {
			return status;
		}
		status = cracen_platform_get_key_slot(psa_get_key_id(attributes), &lifetime,
						      &slot_id);

		if (status != PSA_SUCCESS) {
			return status;
		}

		status = cracen_platform_get_builtin_key(slot_id, &stored_attributes, key_buffer,
							 key_buffer_size, key_buffer_length);

		if (status != PSA_SUCCESS) {
			return status;
		}

		*key_bits = psa_get_key_bits(&stored_attributes);

		return status;
	}
#endif

	if (location != PSA_KEY_LOCATION_LOCAL_STORAGE) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT)) {
		if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type)) {
			return import_ecc_private_key(attributes, data, data_length, key_buffer,
						      key_buffer_size, key_buffer_length, key_bits);
		} else if (PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(key_type)) {
			return import_ecc_public_key(attributes, data, data_length, key_buffer,
						     key_buffer_size, key_buffer_length, key_bits);
		}
	}

	if (PSA_KEY_TYPE_IS_RSA(key_type) &&
	    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_RSA_KEY_PAIR_IMPORT)) {
		return import_rsa_key(attributes, data, data_length, key_buffer, key_buffer_size,
				      key_buffer_length, key_bits);
	}

	if (PSA_KEY_TYPE_IS_SPAKE2P(key_type) && IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
		return import_spake2p_key(attributes, data, data_length, key_buffer,
					  key_buffer_size, key_buffer_length, key_bits);
	}

	if (PSA_KEY_TYPE_IS_SRP(key_type) && IS_ENABLED(PSA_NEED_CRACEN_SRP_6)) {
		return import_srp_key(attributes, data, data_length, key_buffer, key_buffer_size,
				      key_buffer_length, key_bits);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

static psa_status_t generate_rsa_private_key(const psa_key_attributes_t *attributes, uint8_t *key,
					     size_t key_size, size_t *key_length)
{
#if PSA_MAX_RSA_KEY_BITS > 0
	size_t bits = psa_get_key_bits(attributes);
	size_t key_size_bytes = PSA_BITS_TO_BYTES(bits);
	size_t key_size_half = key_size_bytes / 2;

	/* RSA public exponent used in PSA is 65537. We provide this as a 3 byte
	 * big endian array.
	 */
	uint8_t pub_exponent[] = {0x01, 0x00, 0x01};

	struct sitask t;

	psa_status_t status = check_rsa_key_attributes(attributes, bits);

	if (status != PSA_SUCCESS) {
		return status;
	}

	/*
	 * Setup RSA Private key layout.
	 *
	 * RSAPrivateKey ::= SEQUENCE {
	 *		version INTEGER, -- must be 0
	 *		modulus INTEGER, -- n
	 *		publicExponent INTEGER, -- e
	 *		privateExponent INTEGER, -- d
	 *		prime1 INTEGER, -- p
	 *		prime2 INTEGER, -- q
	 *		exponent1 INTEGER, -- d mod (p-1)
	 *		exponent2 INTEGER, -- d mod (q-1)
	 *		coefficient INTEGER, -- (inverse of q) mod p
	 * }
	 */
	uint8_t version_bytes = 0;
	struct sx_buf sequence = {.sz = 0};
	struct sx_buf version = {.bytes = &version_bytes, .sz = sizeof(version_bytes)};

	/* The buffers are first laid out sequentially in the output buffer
	 * by the caller. When the key generation is finished we place the
	 * buffers correctly and write the ASN.1 tag and size fields.
	 */
	struct sx_buf n = {.bytes = key, .sz = key_size_bytes};
	struct sx_buf e = {.bytes = pub_exponent, .sz = sizeof(pub_exponent)};
	struct sx_buf d = {.bytes = n.bytes + n.sz, .sz = key_size_bytes};
	struct sx_buf p = {.bytes = d.bytes + d.sz, .sz = key_size_half};
	struct sx_buf q = {.bytes = p.bytes + p.sz, .sz = key_size_half};
	struct sx_buf dp = {.bytes = q.bytes + q.sz, .sz = key_size_half};
	struct sx_buf dq = {.bytes = dp.bytes + dp.sz, .sz = key_size_half};
	struct sx_buf qinv = {.bytes = dq.bytes + dq.sz, .sz = key_size_half};

	/* Array of buffers in the order they will be serialized. */
	struct sx_buf *buffers[] = {&version, &n, &e, &d, &p, &q, &dp, &dq, &qinv};

	if ((uint8_t *)(qinv.bytes + qinv.sz) > (key + key_size)) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* Generate RSA CRT key. */
	struct si_rsa_key privkey = SI_KEY_INIT_RSACRT(&p, &q, &dp, &dq, &qinv);

	/* The workmem size requirement is twice the key size. */
	uint8_t workmem[PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) * 2] = {};

	si_task_init(&t, workmem, sizeof(workmem));
	si_rsa_create_genprivkey(&t, pub_exponent, sizeof(pub_exponent), key_size_bytes, &privkey);
	si_task_run(&t);

	status = silex_statuscodes_to_psa(si_task_wait(&t));
	safe_memzero(workmem, sizeof(workmem));

	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	/* Generate n and d */
	status = silex_statuscodes_to_psa(sx_rsa_keygen(&p, &q, &e, &n, NULL, &d));

	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	/* In DER encoding all numbers are in 2's complement form. We need to
	 * pad numbers where the first bit is set with 0x0 to encode them as
	 * positive.
	 */

	for (int i = ARRAY_SIZE(buffers) - 1; i >= 0; i--) {
		size_t sign_padding = 0;

		if (buffers[i]->bytes[0] & 0x80) {
			sign_padding = 1;
		}

		sequence.sz += get_asn1_size_with_tag_and_length(buffers[i]->sz + sign_padding);
	}

	size_t total_size = get_asn1_size_with_tag_and_length(sequence.sz);
	size_t offset = 0;

	if (total_size > key_size) {
		status = PSA_ERROR_BUFFER_TOO_SMALL;
		goto error_exit;
	}

	/* Place the buffers from the end and write ASN.1 tag and size fields.
	 */
	for (int i = ARRAY_SIZE(buffers) - 1; i >= 0; i--) {
		uint8_t *new_position = key + total_size - offset - buffers[i]->sz;

		memmove(new_position, buffers[i]->bytes, buffers[i]->sz);
		buffers[i]->bytes = new_position;

		if (buffers[i]->bytes[0] & 0x80) {
			buffers[i]->bytes--;
			buffers[i]->bytes[0] = 0;
			buffers[i]->sz++;
		}

		offset += get_asn1_size_with_tag_and_length(buffers[i]->sz);
		write_tag_and_length(buffers[i], ASN1_INTEGER);
	}

	sequence.bytes = key + total_size - offset;
	write_tag_and_length(&sequence, ASN1_CONSTRUCTED | ASN1_SEQUENCE);

	*key_length = total_size;

	return status;

error_exit:
	*key_length = 0;
	safe_memzero(key, key_size);

	return status;
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_MAX_RSA_KEY_BITS > 0*/
}

psa_status_t generate_key_for_kmu(const psa_key_attributes_t *attributes, uint8_t *key_buffer,
				  size_t key_buffer_size, size_t *key_buffer_length)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);
	uint8_t key[CRACEN_KMU_MAX_KEY_SIZE];
	size_t key_bits;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (PSA_BITS_TO_BYTES(psa_get_key_bits(attributes)) > sizeof(key)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type) &&
	    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE)) {
		status = generate_ecc_private_key(attributes, key, sizeof(key), key_buffer_length);
		if (status != PSA_SUCCESS) {
			return status;
		}
	} else if (key_type == PSA_KEY_TYPE_AES) {
		status = psa_generate_random(key, PSA_BITS_TO_BYTES(psa_get_key_bits(attributes)));
		if (status != PSA_SUCCESS) {
			return status;
		}
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return cracen_import_key(attributes, key, PSA_BITS_TO_BYTES(psa_get_key_bits(attributes)),
				 key_buffer, key_buffer_size, key_buffer_length, &key_bits);
}

psa_status_t cracen_generate_key(const psa_key_attributes_t *attributes, uint8_t *key_buffer,
				 size_t key_buffer_size, size_t *key_buffer_length)
{
	__ASSERT_NO_MSG(key_buffer);
	__ASSERT_NO_MSG(key_buffer_length);

	psa_key_type_t key_type = psa_get_key_type(attributes);
	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));

#if CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
	if (location == PSA_KEY_LOCATION_CRACEN_KMU) {
		return generate_key_for_kmu(attributes, key_buffer, key_buffer_size,
					    key_buffer_length);
	}
#endif

	if (location != PSA_KEY_LOCATION_LOCAL_STORAGE) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (key_buffer_size == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type) &&
	    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE)) {
		return generate_ecc_private_key(attributes, key_buffer, key_buffer_size,
						key_buffer_length);
	}

	if (key_type == PSA_KEY_TYPE_RSA_KEY_PAIR &&
	    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_RSA_KEY_PAIR_GENERATE)) {
		return generate_rsa_private_key(attributes, key_buffer, key_buffer_size,
						key_buffer_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

size_t cracen_get_opaque_size(const psa_key_attributes_t *attributes)
{
	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN) {
		switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes))) {
		case CRACEN_BUILTIN_IDENTITY_KEY_ID:
			if (psa_get_key_type(attributes) ==
			    PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)) {
				return sizeof(ikg_opaque_key);
			}
			break;
		case CRACEN_BUILTIN_MEXT_ID:
		case CRACEN_BUILTIN_MKEK_ID:
			if (psa_get_key_type(attributes) == PSA_KEY_TYPE_AES) {
				return sizeof(ikg_opaque_key);
			}
			break;
#ifdef CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS
		default:
			return cracen_platform_keys_get_size(attributes);
#endif
		}
	}

	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN_KMU) {
		if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
			if (psa_get_key_type(attributes) ==
			    PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)) {
				return PSA_EXPORT_PUBLIC_KEY_OUTPUT_SIZE(
					psa_get_key_type(attributes), psa_get_key_bits(attributes));
			}
			return PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
		} else {
			return sizeof(kmu_opaque_key_buffer);
		}
	}
	return 0;
}

psa_status_t cracen_get_builtin_key(psa_drv_slot_number_t slot_number,
				    psa_key_attributes_t *attributes, uint8_t *key_buffer,
				    size_t key_buffer_size, size_t *key_buffer_length)
{
	/* According to the PSA Crypto Driver specification, the PSA core will set the `id`
	 * and the `lifetime` field of the attribute struct. We will fill all the other
	 * attributes, and update the `lifetime` field to be more specific.
	 */
	switch (slot_number) {
	case CRACEN_IDENTITY_KEY_SLOT_NUMBER:
		psa_set_key_lifetime(attributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
							 PSA_KEY_PERSISTENCE_READ_ONLY,
							 PSA_KEY_LOCATION_CRACEN));
		psa_set_key_type(attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(attributes, 256);
		psa_set_key_algorithm(attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
		psa_set_key_usage_flags(attributes, PSA_KEY_USAGE_SIGN_MESSAGE |
							    PSA_KEY_USAGE_SIGN_HASH |
							    PSA_KEY_USAGE_VERIFY_HASH |
							    PSA_KEY_USAGE_VERIFY_MESSAGE);

		/* According to the PSA Crypto Driver interface proposed document the driver
		 * should fill the attributes even if the buffer of the key is too small. So
		 * we check the buffer here and not earlier in the function.
		 */
		if (key_buffer_size >= cracen_get_opaque_size(attributes)) {
			*key_buffer_length = cracen_get_opaque_size(attributes);
			*((ikg_opaque_key *)key_buffer) =
				(ikg_opaque_key){.slot_number = slot_number,
						 .owner_id = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(
							 psa_get_key_id(attributes))};
			return PSA_SUCCESS;
		} else {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}
		break;

	case CRACEN_MKEK_SLOT_NUMBER:
	case CRACEN_MEXT_SLOT_NUMBER:
		psa_set_key_lifetime(attributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
							 PSA_KEY_PERSISTENCE_READ_ONLY,
							 PSA_KEY_LOCATION_CRACEN));
		psa_set_key_type(attributes, PSA_KEY_TYPE_AES);
		psa_set_key_bits(attributes, 256);
		psa_set_key_algorithm(attributes, PSA_ALG_SP800_108_COUNTER_CMAC);
		psa_set_key_usage_flags(attributes,
					PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_VERIFY_DERIVATION);

		/* See comment about the placement of this check in the previous switch
		 * case.
		 */
		if (key_buffer_size >= cracen_get_opaque_size(attributes)) {
			*key_buffer_length = cracen_get_opaque_size(attributes);
			*((ikg_opaque_key *)key_buffer) =
				(ikg_opaque_key){.slot_number = slot_number,
						 .owner_id = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(
							 psa_get_key_id(attributes))};
			return PSA_SUCCESS;
		} else {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

	default:
#if CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
		return cracen_kmu_get_builtin_key(slot_number, attributes, key_buffer,
						  key_buffer_size, key_buffer_length);
#elif CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS
		return cracen_platform_get_builtin_key(slot_number, attributes, key_buffer,
						       key_buffer_size, key_buffer_length);
#else
		return PSA_ERROR_DOES_NOT_EXIST;
#endif
	}
}

psa_status_t mbedtls_psa_platform_get_builtin_key(mbedtls_svc_key_id_t key_id,
						  psa_key_lifetime_t *lifetime,
						  psa_drv_slot_number_t *slot_number)
{
	switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id)) {
	case CRACEN_BUILTIN_IDENTITY_KEY_ID:
		*slot_number = CRACEN_IDENTITY_KEY_SLOT_NUMBER;
		break;
	case CRACEN_BUILTIN_MKEK_ID:
		*slot_number = CRACEN_MKEK_SLOT_NUMBER;
		break;
	case CRACEN_BUILTIN_MEXT_ID:
		*slot_number = CRACEN_MEXT_SLOT_NUMBER;
		break;
	default:
#if CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
		return cracen_kmu_get_key_slot(key_id, lifetime, slot_number);
#elif CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS
		return cracen_platform_get_key_slot(key_id, lifetime, slot_number);
#else
		return PSA_ERROR_DOES_NOT_EXIST;
#endif
	};

	*lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(PSA_KEY_PERSISTENCE_READ_ONLY,
								   PSA_KEY_LOCATION_CRACEN);

	return PSA_SUCCESS;
}

psa_status_t cracen_export_key(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			       size_t key_buffer_size, uint8_t *data, size_t data_size,
			       size_t *data_length)
{
#ifdef CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
	int status;
	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));

	if (location == PSA_KEY_LOCATION_CRACEN_KMU) {
		/* The keys will already be in the key buffer as they got loaded their by a previous
		 * call to cracen_get_builtin_key or cached in the memory.
		 */
		psa_ecc_family_t ecc_fam =
			PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes));
		if (ecc_fam == PSA_ECC_FAMILY_TWISTED_EDWARDS ||
		    ecc_fam == PSA_ECC_FAMILY_SECP_R1) {
			memcpy(data, key_buffer, key_buffer_size);
			*data_length = key_buffer_size;
			return PSA_SUCCESS;
		}

		size_t key_out_size = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));

		if (key_out_size > data_size) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		/* The kmu_push_area is guarded by the symmetric mutex since it is the most common
		 * use case. Here the decision was to avoid defining another mutex to handle the
		 * push buffer for the rest of the use cases.
		 */
		nrf_security_mutex_lock(&cracen_mutex_symmetric);
		status = cracen_kmu_prepare_key(key_buffer);
		if (status == SX_OK) {
			memcpy(data, kmu_push_area, key_out_size);
			*data_length = key_out_size;
		}

		(void)cracen_kmu_clean_key(key_buffer);
		nrf_security_mutex_unlock(&cracen_mutex_symmetric);

		return silex_statuscodes_to_psa(status);
	}
#endif

	return PSA_ERROR_DOES_NOT_EXIST;
}

psa_status_t cracen_copy_key(psa_key_attributes_t *attributes, const uint8_t *source_key,
			     size_t source_key_length, uint8_t *target_key_buffer,
			     size_t target_key_buffer_size, size_t *target_key_buffer_length)
{
#ifdef CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));

	/* PSA core only invokes this if source location matches target location.
	 * Whether copy usage is allowed has been validated at this point.
	 */
	if (location != PSA_KEY_LOCATION_CRACEN_KMU) {
		return PSA_ERROR_DOES_NOT_EXIST;
	}

	if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
		size_t key_bits;

		return cracen_import_key(attributes, source_key, source_key_length,
					 target_key_buffer, target_key_buffer_size,
					 target_key_buffer_length, &key_bits);
	}

	int status;
	psa_status_t psa_status;
	size_t key_size = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));

	nrf_security_mutex_lock(&cracen_mutex_symmetric);
	status = cracen_kmu_prepare_key(source_key);

	if (status == SX_OK) {
		size_t key_bits;

		psa_status = cracen_import_key(attributes, kmu_push_area, key_size,
					       target_key_buffer, target_key_buffer_size,
					       target_key_buffer_length, &key_bits);
	}

	(void)cracen_kmu_clean_key(source_key);
	nrf_security_mutex_unlock(&cracen_mutex_symmetric);

	if (status != SX_OK) {
		return silex_statuscodes_to_psa(status);
	}

	return psa_status;
#endif

	return PSA_ERROR_DOES_NOT_EXIST;
}

psa_status_t cracen_destroy_key(const psa_key_attributes_t *attributes)
{
#ifdef CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
	return cracen_kmu_destroy_key(attributes);
#endif

	return PSA_ERROR_DOES_NOT_EXIST;
}
