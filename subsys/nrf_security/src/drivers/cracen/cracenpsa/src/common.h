/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "kmu.h"
#include "cracen_psa.h"
#include "sxsymcrypt/internal.h"

#include <stddef.h>
#include <zephyr/sys/util.h>
#include <sicrypto/sicrypto.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <sxsymcrypt/sha1.h>
#include <sxsymcrypt/sha2.h>
#include <sxsymcrypt/sha3.h>

/* RFC5480 - first byte of ECC public key that indicates that the key
 * is uncompressed
 */
#define SI_ECC_PUBKEY_UNCOMPRESSED (0x04)

/* Cracen supports max 521 bits curves, the private key for such a curve
 * is 66 bytes. The public key in PSA APIs is stored with the
 * the uncompressed representation which means that it has 0x4
 * as the first byte and then the X and Y coordinates, so the
 * public key max size is 1 + 2 * CRACEN_MAX_ECC_PRIV_KEY_BYTES.
 */
#define CRACEN_MAC_ECC_PRIVKEY_BYTES (66)
#define CRACEN_X25519_KEY_SIZE_BYTES (32)
#define CRACEN_X448_KEY_SIZE_BYTES   (56)

__attribute__((warn_unused_result)) psa_status_t silex_statuscodes_to_psa(int ret);

__attribute__((warn_unused_result)) psa_status_t
hash_get_algo(psa_algorithm_t alg, const struct sxhashalg **sx_hash_algo);

/*!
 * \brief Get Cracen curve object based on the PSA attributes.
 *
 * \param[in]  curve_family PSA curve family.
 * \param[in]  curve_bits   Curve bits.
 * \param[out] sicurve      Pointer to curve struct for Cracen.
 *
 * \return PSA_SUCCESS on success or a valid PSA error code.
 */
psa_status_t cracen_ecc_get_ecurve_from_psa(psa_ecc_family_t curve_family, size_t curve_bits,
					    const struct sx_pk_ecurve **sicurve);

/*!
 * \brief Check if a curve is of Weierstrass type.
 *
 * \param[in]  curve The PSA curve family.
 *
 * \return TRUE if the curve is in Weierstrass form, FALSE if it is not.
 */
bool cracen_ecc_curve_is_weierstrass(psa_ecc_family_t curve);

/*!
 * \brief Calculate the expected public key size for Weierstrass curves.
 *
 * \param[in] priv_key_size The size of the private key in bytes.
 *
 * \return The expected size of the public key in bytes as described
 *         by the PSA APIs. The representation of the public key for
 *         Weierstrass curves in PSA is 0x4 | X | Y.
 */
static inline size_t cracen_ecc_wstr_expected_pub_key_bytes(size_t priv_key_size)
{
	return (1 + (2 * priv_key_size));
}

/*!
 * \brief Check ECC public key for validity based on the 800-56A.
 *
 * \note  ECDH keys need to be checked based on the NIST 800-56A document.
 *        There are two test methods depending on the type of the curve,
 *        the partial and full check. This function we want to implement the
 * full check. The full check has 4 requirements (section 5.6.2.3.3): 1) Verify
 * that pnt is not the identity/infinity point. 2) Verify that x and y of pnt
 * are integers in the interval [0, p − 1] 3) Verify that pnt is on the curve 4)
 * Compute n * pnt and verify == identity/infinity point
 *
 * \param[in] curve  The curve of the public key.
 * \param[in] in_pnt The public key to check.
 *
 * \return  PSA_SUCCESS if the public key passed the check, a valid
 *          PSA error code otherwise.
 *
 */
psa_status_t cracen_ecc_check_public_key(const struct sx_pk_ecurve *curve,
					 const sx_pk_affine_point *in_pnt);

/**
 * \brief Tries to extract an RSA key from ASN.1.
 *
 * \param[out] rsa                 Resulting RSA key.
 * \param[in]  extract_pubkey      true to extract public key. false to extract private key.
 * \param[in]  is_key_pair         false if key is a public key. true if it is an RSA key pair.
 * \param[in]  key                 Buffer containing RSA key in ASN.1 format.
 * \param[in]  keylen              Length of buffer in bytes.
 * \param[out] modulus             Modulus (n) operand of n.
 * \param[out] exponent            Public or private exponent, depending on \ref extract_pubkey.
 *
 * \return sicrypto statuscode.
 */
int cracen_signature_get_rsa_key(struct si_rsa_key *rsa, bool extract_pubkey, bool is_key_pair,
				 const unsigned char *key, size_t keylen, struct sx_buf *modulus,
				 struct sx_buf *exponent);

/**
 * \brief Extracts an ASN.1 integer (excluding the leading zeros) and sets up an sx_buff pointing to
 * it.
 *
 * \param[in,out]  p               ASN.1 buffer, will be updated to point to the next ASN.1 element.
 * \param[in]      end             The end of the ASN.1 buffer.
 * \param[out]     op              The sx_buff operand which will hold the pointer to the extracted
 * ASN.1 integer.
 *
 * \retval SX_OK on success.
 * \retval SX_ERR_INVALID_PARAM if the ASN.1 integer cannot be extracted.
 */
int cracen_signature_asn1_get_operand(unsigned char **p, const unsigned char *end,
				      struct sx_buf *op);

/**
 * @brief Use psa_generate_random up to generate a random number in the range [1, upperlimit).
 *
 * @param[out] n           Output number.
 * @param[in]  sz          Size of number in bytes.
 * @param[in]  upperlimit  Upper limit for generated number.
 * @param[in]  retrylimit  Maximum number of attempts to generate a number in the range.
 *
 * @note Output number and upper limit must be big endian numbers of size @ref sz.
 *
 * @return psa_status_t
 */
psa_status_t rnd_in_range(uint8_t *n, size_t sz, const uint8_t *upperlimit, size_t retrylimit);

/**
 * @brief XOR two byte buffers and store result in first buffer
 *
 * @param[in,out] a First buffer of size sz
 * @param[in] b Second buffer of size sz
 * @param[in] sz Size of the buffers
 */
void cracen_xorbytes(char *a, const char *b, size_t sz);

/**
 * @brief Loads key buffer and attributes.
 *
 * @return psa_status_t
 */
psa_status_t cracen_load_keyref(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, struct sxkeyref *k);

/**
 * @brief Do ECB operation.
 *
 * @return psa_status_t
 */
psa_status_t cracen_cipher_crypt_ecb(const struct sxkeyref *key, const uint8_t *input,
				     size_t input_length, uint8_t *output, size_t output_size,
				     size_t *output_length, enum cipher_operation dir,
				     bool aes_countermeasures);
