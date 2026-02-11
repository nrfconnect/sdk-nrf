/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cracen/cracen_kmu.h>
#include <cracen_psa.h>
#include <sxsymcrypt/internal.h>

#include <stddef.h>
#include <stdint.h>
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

psa_status_t check_wstr_pub_key_data(psa_algorithm_t key_alg, psa_ecc_family_t curve,
					    size_t key_bits, const uint8_t *data,
					    size_t data_length);

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
					 const sx_pk_const_affine_point *in_pnt);

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

/**
 * @brief Check if the value is a quadratic residue modulo p,
 *	  where p is an EC prime.
 *
 * @param[in,out] req Acquired acceleration request for this operation
 * @param[in] curve_prime EC prime.S
 * @param[in] value       Value to check.
 * @param[out] is_qr      Result of the check. This outputs true if the value
 *			  is a quadratic residue; false otherwise.
 *
 * @return psa_status_t
 */
psa_status_t cracen_ecc_is_quadratic_residue(sx_pk_req *req,
						const sx_const_op *curve_prime,
						const sx_const_op *value, bool *is_qr);

/**
 * @brief Derive an element of an ECC group (point on the curve) from the given hash.
 *	  The function implements the SSWU algorithm, as described in the RFC 9380 document.
 *
 * @param[in,out] req Acquired acceleration request for this operation
 * @param[in] curve_family PSA curve family.
 * @param[in] curve_bits   Curve bits.
 * @param[in] u            Hash to use.
 * @param[out] result      Result (point on ECC curve).
 *
 * @return psa_status_t
 */
psa_status_t cracen_ecc_h2e_sswu(sx_pk_req *req, psa_ecc_family_t curve_family,
				size_t curve_bits, const sx_const_op *u, const sx_op *result);
