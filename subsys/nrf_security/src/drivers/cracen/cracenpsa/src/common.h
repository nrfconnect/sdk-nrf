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
#include <silexpk/sxbuf/sxbufop.h>
#include <sxsymcrypt/hashdefs.h>
#include <silexpk/core.h>

/* RFC5480 - first byte of ECC public key that indicates that the key
 * is uncompressed, per SEC1 v2 spec
 */
#define CRACEN_ECC_PUBKEY_UNCOMPRESSED (0x04)

/* Cracen supports max 521 bits curves, the private key for such a curve
 * is 66 bytes. The public key in PSA APIs is stored with the
 * the uncompressed representation which means that it has 0x4
 * as the first byte and then the X and Y coordinates, so the
 * public key max size is 1 + 2 * CRACEN_MAX_ECC_PRIV_KEY_BYTES.
 */
#define CRACEN_MAC_ECC_PRIVKEY_BYTES (66)
#define CRACEN_X25519_KEY_SIZE_BYTES (32)
#define CRACEN_X448_KEY_SIZE_BYTES   (56)

/* Size of the workmem of the MGF1XOR function. */
#define MGF1XOR_WORKMEM_SZ(digestsz) ((digestsz) + 4)

/* Return a pointer to the part of workmem that is specific to RSA. */
static inline uint8_t *cracen_get_rsa_workmem_pointer(uint8_t *workmem, size_t digestsz)
{
	return workmem + MGF1XOR_WORKMEM_SZ(digestsz);
}

static inline size_t cracen_get_rsa_workmem_size(size_t workmemsz, size_t digestsz)
{
	return workmemsz - MGF1XOR_WORKMEM_SZ(digestsz);
}

typedef struct {
	uint8_t slot_number;
	uint8_t owner_id;
} ikg_opaque_key;

__attribute__((warn_unused_result)) psa_status_t silex_statuscodes_to_psa(int ret);

__attribute__((warn_unused_result)) psa_status_t
hash_get_algo(psa_algorithm_t alg, const struct sxhashalg **sx_hash_algo);

enum asn1_tags {
	ASN1_SEQUENCE = 0x10,
	ASN1_INTEGER = 0x2,
	ASN1_CONSTRUCTED = 0x20,
	ASN1_BIT_STRING = 0x3
};

/*!
 * \brief Get Cracen curve object based on the PSA attributes.
 *
 * \param[in]  curve_family PSA curve family.
 * \param[in]  curve_bits   Curve bits.
 * \param[out] sicurve      Pointer to curve struct for Cracen.
 *
 * \return PSA_SUCCESS on success or a valid PSA status code.
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
 *          PSA status code otherwise.
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
 * \return sxsymcrypt status code.
 */
int cracen_signature_get_rsa_key(struct cracen_rsa_key *rsa, bool extract_pubkey, bool is_key_pair,
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
int cracen_signature_asn1_get_operand(uint8_t **p, const uint8_t *end, struct sx_buf *op);

/**
 * @brief Use cracen_get_random up to generate a random number in the range [1, upperlimit).
 *
 * @param[out] n           Output number.
 * @param[in]  sz          Size of number in bytes.
 * @param[in]  upperlimit  Upper limit for generated number.
 * @param[in]  retrylimit  Maximum number of attempts to generate a number in the range.
 *
 * @note Output number and upper limit must be big endian numbers of size @ref sz.
 *
 * @return PSA status code.
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
 * @return PSA status code.
 */
psa_status_t cracen_load_keyref(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, struct sxkeyref *k);

/**
 * @brief Do ECB operation.
 *
 * @return PSA status code.
 */
psa_status_t cracen_cipher_crypt_ecb(const struct sxkeyref *key, const uint8_t *input,
				     size_t input_length, uint8_t *output, size_t output_size,
				     size_t *output_length, enum cipher_operation dir);

/**
 * @brief Prepare ik key.
 *
 * @param user_data    Owner ID.
 *
 * @return sxsymcrypt status code.
 */
int cracen_prepare_ik_key(const uint8_t *user_data);

/**
 * @brief Compute v = v + summand.
 * \note  v is an unsigned integer stored as a big endian byte array of sz bytes.
 *        Summand must be less than or equal to the maximum value of a size_t minus 255.
 *        The final carry is discarded: addition is modulo 2^(sz*8).
 *
 * @param v		Big-endian Value
 * @param v_size	size of v
 * @param summand	Summand.
 *
 */
void cracen_be_add(uint8_t *v, size_t v_size, size_t summand);

/**
 * @brief Big-Endian compare with carry.
 * \note
 *
 * @param a		First value to be compared
 * @param b		Second value to be compared
 * @param size		Size of a and b.
 * @param carry		Value of the carry.
 *
 * \retval 0 if equal.
 * \retval 1 if a > b.
 * \retval -1 if a < b.
 */
int cracen_be_cmp(const uint8_t *a, const uint8_t *b, size_t sz, int carry);

/**
 * @brief Hash several elements at different locations in memory
 *
 * @param inputs[in]		Array of pointers to elements that will be hashed.
 * @param input_lengths[in]	Array of lengths of elements to be hashed.
 * @param input_count[in]	Number of elements to be hashed.
 * @param hashalg[in]		Hash algorithm to be used in sxhashalg format.
 * @param digest[out]		Buffer of at least sx_hash_get_alg_digestsz(hashalg) bytes.
 *
 * @return sxsymcrypt status code.
 */
int cracen_hash_all_inputs(const uint8_t *inputs[], const size_t input_lengths[],
			   size_t input_count, const struct sxhashalg *hashalg, uint8_t *digest);

/**
 * @brief Hash several elements at different locations in memory with a previously created hash
 * context(sxhash)
 *
 * @param sxhashopctx[in]	Pointer to the sxhash context.
 * @param inputs[in]		Array of pointers to elements that will be hashed.
 * @param input_lengths[in]	Array of lengths of elements to be hashed.
 * @param input_count[in]	Number of elements to be hashed.
 * @param hashalg[in]		Hash algorithm to be used in sxhashalg format.
 * @param digest[out]		Buffer of at least sx_hash_get_alg_digestsz(hashalg) bytes.
 *
 * @return sxsymcrypt status code.
 */
int cracen_hash_all_inputs_with_context(struct sxhash *sxhashopctx, const uint8_t *inputs[],
					const size_t input_lengths[], size_t input_count,
					const struct sxhashalg *hashalg, uint8_t *digest);

/**
 * @brief Hash a single element
 *
 * @param input[in]		Pointer to the element that will be hashed.
 * @param input_length[in]	Length of the element to be hashed.
 * @param hashalg[in]		Hash algorithm to be used in sxhashalg format.
 * @param digest[out]		Buffer of at least sx_hash_get_alg_digestsz(hashalg) bytes.
 *
 * @return sxsymcrypt status code.
 */
int cracen_hash_input(const uint8_t *input, const size_t input_length,
		      const struct sxhashalg *hashalg, uint8_t *digest);

/**
 * @brief Hash a single element with a previously created hash context(sxhash)
 *
 * @param sxhashopctx[in]	Pointer to the sxhash context.
 * @param input[in]		Pointer to element that will be hashed.
 * @param input_length[in]	Length of element to be hashed.
 * @param hashalg[in]		hash algorithm to be used in sxhashalg format.
 * @param digest[out]		Buffer of at least sx_hash_get_alg_digestsz(hashalg) bytes.
 *
 * @return sxsymcrypt status code.
 */
int cracen_hash_input_with_context(struct sxhash *hashopctx, const uint8_t *input,
				   const size_t input_length, const struct sxhashalg *hashalg,
				   uint8_t *digest);

/**
 * @brief Generate a random number within the specified range.
 *
 * \note  This function generates a random number strictly less than the given upper limit (`n`).
 *        The generated random number is evenly distributed over the range [0, n-1]. If the range
 *        is invalid (e.g., `n` is zero, less than three, or even), an error code is returned.
 *
 * @param n[in]         Pointer to a buffer holding the upper limit of the random range.
 *                      The upper limit must be a non-zero odd number.
 * @param nsz[in]       Size of the upper limit buffer in bytes.
 * @param out[out]      Buffer to store the generated random number.
 *                      The size of `out` should be at least `nsz`.
 *
 * @return sxsymcrypt status code.
 */
int cracen_get_rnd_in_range(const uint8_t *n, size_t nsz, uint8_t *out);

/**
 * @brief Performs `input` modulo the order of the NIST p256 curve
 *
 * @param input Input for the modulo operation.
 * @param input_size Input size in bytes.
 * @param output Output of the modulo operation.
 * @param output_size Output size in bytes.
 *
 * @return psa_status_t
 */
psa_status_t cracen_ecc_reduce_p256(const uint8_t *input, size_t input_size, uint8_t *output,
				    size_t output_size);

/** Modular exponentiation (base^key mod n).
 *
 * This function is used by both the sign and the verify functions. Note: if the
 * base is greater than the modulus, SilexPK will return the SX_ERR_OUT_OF_RANGE
 * status code.
 */
int cracen_rsa_modexp(struct sx_pk_acq_req *pkreq, struct sx_pk_slot *inputs,
		      struct cracen_rsa_key *rsa_key, uint8_t *base, size_t basez, int *sizes);

#define CRACEN_KEY_INIT_RSA(mod, expon)                                                            \
	(struct cracen_rsa_key)                                                                    \
	{                                                                                          \
		.cmd = SX_PK_CMD_MOD_EXP, .slotmask = (1 | (1 << 2)), .dataidx = 1,                \
		{                                                                                  \
			mod, expon                                                                 \
		}                                                                                  \
	}

/** Initialize an RSA CRT key consisting of 2 primes and derived values.
 *
 * The 2 primes (p and q) must comply with the rules laid out in
 * the latest version of FIPS 186. This includes that the 2 primes must
 * have the highest bit set in their most significant byte. The full
 * key size in bits must be a multiple of 8.
 */
#define CRACEN_KEY_INIT_RSACRT(p, q, dp, dq, qinv)                                                 \
	(struct cracen_rsa_key)                                                                    \
	{                                                                                          \
		.cmd = SX_PK_CMD_MOD_EXP_CRT, .slotmask = (0x3 | (0x7 << 3)), .dataidx = 2,        \
		{                                                                                  \
			p, q, dp, dq, qinv                                                         \
		}                                                                                  \
	}
