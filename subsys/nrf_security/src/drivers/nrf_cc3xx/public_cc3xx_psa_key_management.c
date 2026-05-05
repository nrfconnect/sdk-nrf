/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

/** \file cc3xx_psa_key_generation.c
 *
 * This file contains the implementation of the entry points associated to the
 * key generation (i.e. random generation and extraction of public keys) as
 * described by the PSA Cryptoprocessor Driver interface specification
 *
 */
#include <psa/crypto.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cc3xx_psa_key_generation.h>
#include <cc3xx_asn1_util.h>
#include <cc3xx_psa_rsa_util.h>
#include <zephyr/sys/util.h>

static size_t calculate_rsa_key_size(const uint8_t *key_data, size_t key_data_len, bool is_keypair)
{
	int ret;
	uint8_t **key_start_pnt = (unsigned char **)&key_data;
	uint8_t *key_end_pnt = (unsigned char *)key_data + key_data_len;
	size_t len;

	/* Move the pointer after the sequence */
	ret = cc3xx_asn1_get_tag(key_start_pnt, key_end_pnt, &len,
				 0x20 | /* Constructed */
				 0x10); /* Sequence */
	if (ret < 0) {
		return 0;
	}

	if (is_keypair) {
		/* The key pair has a version number as an integer */
		ret = cc3xx_asn1_get_tag(key_start_pnt, key_end_pnt, &len, 0x2); /* Integer */
		if (ret < 0) {
			return 0;
		}

		*key_start_pnt += len;
	}

	/* Get the modulus n */
	ret = cc3xx_asn1_get_tag(key_start_pnt, key_end_pnt, &len, 0x2); /* Integer */
	if (ret < 0) {
		return 0;
	}

	if (*key_start_pnt[0] == 0x00) {
		len -= 1;
	}

	/* The buffer size for the modulus n is the RSA key size */
	return PSA_BYTES_TO_BITS(len);
}

static size_t calculate_ecc_key_size(size_t key_data_len, psa_ecc_family_t curve_family,
		     bool is_keypair)
{
	/**
	 * For montgomery and twisted edwards curve the bits should be equal to the buffer -1.
	 * The -1 is because the key size is 255 bits but the buffer size will always be a multiple
	 * of 8 (256 bits). Private and public keys use the same structure and size so no need to
	 * distinguish between them.
	 */
	if (curve_family == PSA_ECC_FAMILY_MONTGOMERY ||
	    curve_family == PSA_ECC_FAMILY_TWISTED_EDWARDS) {
		return PSA_BYTES_TO_BITS(key_data_len) - 1;
	}

	/* Keypairs are generaly simpler because only the private key is used */
	if (is_keypair) {
		/* 521 bit curves are a special case since they are not a multiple of 8 */
		if (key_data_len == PSA_BITS_TO_BYTES(521)) {
			return 521;
		}

		return PSA_BYTES_TO_BITS(key_data_len);
	}

	/**
	 * For the SECP_R1, SECP_R2, SECP_K1 the public key has the structure
	 * 04 | X | Y
	 * and the bit size of the curve is equivalent to the X/Y length.
	 */

	/* 521 bit curves are a special case since they are not a multiple of 8 */
	if (key_data_len == (1 + 2 * PSA_BITS_TO_BYTES(521))) {
		return 521;
	}

	return PSA_BYTES_TO_BITS(key_data_len - 1) / 2;
}

static size_t calculate_key_size(psa_key_type_t key_type, const uint8_t *key_buff,
		 size_t key_buff_len)
{
	/* For unstructured keys we can only rely on the buffer size to calculate the key size */
	if (PSA_KEY_TYPE_IS_UNSTRUCTURED(key_type)) {
		return PSA_BYTES_TO_BITS(key_buff_len);
	}

	/* For RSA keys we need to parse the ASN1 and get the modulus n */
	if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_RSA_ANY) && PSA_KEY_TYPE_IS_RSA(key_type)) {
		return calculate_rsa_key_size(key_buff, key_buff_len,
					      key_type == PSA_KEY_TYPE_RSA_KEY_PAIR);
	}

	if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_ECC_ANY) && PSA_KEY_TYPE_IS_ECC(key_type)) {
		psa_ecc_family_t curve_family = PSA_KEY_TYPE_ECC_GET_FAMILY(key_type);

		return calculate_ecc_key_size(key_buff_len, curve_family,
					      PSA_KEY_TYPE_IS_KEY_PAIR(key_type));
	}

	return PSA_ERROR_INVALID_ARGUMENT;
}

static psa_status_t validate_ecc_key_size(psa_ecc_family_t curve, size_t bits)
{
	switch (curve) {
	case PSA_ECC_FAMILY_SECP_R1:
		if (bits != 160 && bits != 192 && bits != 224 && bits != 256 && bits != 384 &&
		    bits != 521) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		return PSA_SUCCESS;
	case PSA_ECC_FAMILY_SECP_K1:
		if (bits != 160 && bits != 192 && bits != 224 && bits != 256) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		return PSA_SUCCESS;
	case PSA_ECC_FAMILY_MONTGOMERY:
	case PSA_ECC_FAMILY_TWISTED_EDWARDS:
		if (bits != 255) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		return PSA_SUCCESS;
	case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
		/**
		 * The driver should only return not supported, while the core handles
		 * invalid checks, as it might choose another driver if e.g. a 192r1 is
		 * chosen
		 */
		if (bits != 256) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		return PSA_SUCCESS;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

static psa_status_t cc3xx_generate_key_ecc(const psa_key_attributes_t *attributes,
					   uint8_t *key_buffer, size_t key_buffer_size,
					   size_t *key_buffer_length)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);
	psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(key_type);

	if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_ECC_WEIERSTRASS) &&
	    PSA_ECC_FAMILY_IS_WEIERSTRASS(curve)) {
		return cc3xx_internal_gen_ecc_wstr_keypair(attributes, key_buffer, key_buffer_size,
							   key_buffer_length);
	} else if (IS_ENABLED(PSA_NEED_CC3XX_ECDH_MONTGOMERY_255) &&
		   curve == PSA_ECC_FAMILY_MONTGOMERY) {
		return cc3xx_internal_gen_ecc_mont_keypair(attributes, key_buffer, key_buffer_size);
	} else if (IS_ENABLED(PSA_NEED_CC3XX_PURE_EDDSA_TWISTED_EDWARDS_255) &&
		   curve == PSA_ECC_FAMILY_TWISTED_EDWARDS) {
		return cc3xx_internal_gen_ecc_edwards_keypair(attributes, key_buffer,
							      key_buffer_size);
	}

	return PSA_ERROR_NOT_SUPPORTED;

}

static psa_status_t cc3xx_import_key_ecc(const psa_key_attributes_t *attributes, size_t attr_bits,
					 const uint8_t *data, size_t data_length,
					 uint8_t *key_buffer, size_t key_buffer_size,
					 size_t *key_buffer_length, size_t *key_bits)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);
	psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(key_type);

	if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_ECC_WEIERSTRASS) &&
	    PSA_ECC_FAMILY_IS_WEIERSTRASS(curve)) {
		return cc3xx_internal_check_wrst_key(curve, attr_bits, key_type, data, data_length);
	} else if (IS_ENABLED(PSA_NEED_CC3XX_ECDH_MONTGOMERY_255) &&
		 curve == PSA_ECC_FAMILY_MONTGOMERY) {
		return cc3xx_internal_check_mont_key(key_type, data, data_length);
	} else if (IS_ENABLED(PSA_NEED_CC3XX_PURE_EDDSA_TWISTED_EDWARDS_255) &&
		   curve == PSA_ECC_FAMILY_TWISTED_EDWARDS) {
		return cc3xx_internal_check_edw_key(key_type, data, data_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;

}

static psa_status_t cc3xx_export_public_key_ecc(const psa_key_attributes_t *attributes,
						const uint8_t *key_buffer, size_t key_buffer_size,
						uint8_t *data, size_t data_size,
						size_t *data_length)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);
	psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(key_type);

	if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_ECC_WEIERSTRASS) &&
	    PSA_ECC_FAMILY_IS_WEIERSTRASS(curve)) {
		return cc3xx_internal_export_ecc_wrst_public_key(
			attributes, key_buffer, key_buffer_size,
			data, data_size, data_length);
	} else if (IS_ENABLED(PSA_NEED_CC3XX_ECDH_MONTGOMERY_255) &&
		   curve == PSA_ECC_FAMILY_MONTGOMERY) {
		return cc3xx_internal_export_ecc_mont_public_key(
			attributes, key_buffer, key_buffer_size,
			data, data_size, data_length);
	} else if (IS_ENABLED(PSA_NEED_CC3XX_PURE_EDDSA_TWISTED_EDWARDS_255) &&
		   curve == PSA_ECC_FAMILY_TWISTED_EDWARDS) {
		return cc3xx_internal_export_ecc_edwards_public_key(
			attributes, key_buffer, key_buffer_size,
			data, data_size, data_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

static psa_status_t validate_key_size(psa_key_type_t type, size_t bits)
{
	if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_ECC_ANY) && PSA_KEY_TYPE_IS_ECC(type)) {
		psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(type);

		return validate_ecc_key_size(curve, bits);
	}

	if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_RSA_ANY)) {
		switch (type) {
		case PSA_KEY_TYPE_RSA_KEY_PAIR:
		case PSA_KEY_TYPE_RSA_PUBLIC_KEY:
			return cc3xx_check_rsa_key_size(bits);
		default:
			break;
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

/** \defgroup psa_key_generation PSA driver entry points for key handling
 *
 *  Entry points for random key generation and key format manipulation and
 *  translation as described by the PSA Cryptoprocessor Driver interface
 *  specification
 *
 *  @{
 */
psa_status_t cc3xx_generate_key(const psa_key_attributes_t *attributes, uint8_t *key_buffer,
				size_t key_buffer_size, size_t *key_buffer_length)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);
	size_t key_bits = psa_get_key_bits(attributes);
	psa_status_t err = PSA_ERROR_NOT_SUPPORTED;

	if (key_buffer_size < PSA_BITS_TO_BYTES(key_bits)) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	err = validate_key_size(key_type, key_bits);
	if (err != PSA_SUCCESS) {
		return err;
	}

	if (!PSA_KEY_TYPE_IS_KEY_PAIR(key_type)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_ECC_ANY) && PSA_KEY_TYPE_IS_ECC(key_type)) {
		return cc3xx_generate_key_ecc(attributes, key_buffer, key_buffer_size,
					      key_buffer_length);
	} else if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_RSA_ANY) && PSA_KEY_TYPE_IS_RSA(key_type)) {
		return cc3xx_internal_gen_rsa_keypair(attributes, key_buffer,
						      key_buffer_size, key_buffer_length);
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t cc3xx_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
		  size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
		  size_t *key_buffer_length, size_t *key_bits)
{
	psa_status_t err = PSA_ERROR_CORRUPTION_DETECTED;
	size_t attr_bits = psa_get_key_bits(attributes);
	psa_key_type_t key_type = psa_get_key_type(attributes);

	if (data == NULL || key_buffer == NULL || key_buffer_length == NULL || key_bits == NULL ||
	    data_length == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_length > key_buffer_size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* if the bits are not set we need to calculate it */
	if (attr_bits == 0) {
		attr_bits = calculate_key_size(key_type, data, data_length);
	}

	/* Validate ECC or RSA key size */
	err = validate_key_size(key_type, attr_bits);
	if (err != PSA_SUCCESS) {
		return err;
	}

	if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_ECC_ANY) && PSA_KEY_TYPE_IS_ECC(key_type)) {
		err = cc3xx_import_key_ecc(attributes, attr_bits, data, data_length, key_buffer,
					   key_buffer_size, key_buffer_length, key_bits);

		if (err != PSA_SUCCESS) {
			return err;
		}
	}

	memcpy(key_buffer, data, data_length);
	*key_bits = attr_bits;
	*key_buffer_length = data_length;

	return PSA_SUCCESS;
}

psa_status_t cc3xx_export_public_key(const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     uint8_t *data, size_t data_size, size_t *data_length)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);
	size_t key_bits = psa_get_key_bits(attributes);
	psa_status_t err = PSA_ERROR_CORRUPTION_DETECTED;

	/* Initialise the return value to 0 */
	*data_length = 0;

	err = validate_key_size(key_type, key_bits);
	if (err != PSA_SUCCESS) {
		return err;
	}

	if (PSA_KEY_TYPE_IS_PUBLIC_KEY(key_type)) {
		/* Revert to software driver when the key is public (no conversion needed) */
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_ECC_ANY) && PSA_KEY_TYPE_IS_ECC(key_type)) {
		return cc3xx_export_public_key_ecc(attributes, key_buffer, key_buffer_size,
						   data, data_size, data_length);
	} else if (IS_ENABLED(PSA_NEED_CC3XX_KEY_TYPE_RSA_ANY) && PSA_KEY_TYPE_IS_RSA(key_type)) {
		return cc3xx_rsa_psa_priv_to_psa_publ((uint8_t *)key_buffer, key_buffer_size,
						      data, data_size, data_length);
	} else {
		/* ECC type did not match supported features */
		return PSA_ERROR_NOT_SUPPORTED;
	}
}
