/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal definitions for the CRACEN ML-DSA (FIPS 204) implementation.
 *
 * Shared types, global constants, and the prototypes of the internal helpers.
 * Helper functions are named after the FIPS 204 algorithm they implement,
 * so that the code can be checked against the specification directly. Not part
 * of the public driver API.
 */

#ifndef CRACEN_ML_DSA_INTERNAL_H
#define CRACEN_ML_DSA_INTERNAL_H

#include <psa/crypto_values.h>
#include <stddef.h>
#include <stdint.h>

/** The constant-time properties of this implementation are claimed for Cortex-M33 only.
 *
 *  - On Cortex-M33 MUL, MLA and UMULL are fixed latency, which is what lets the Montgomery
 *    reduction run in constant time.
 *  - Division operations (UDIV and SDIV) terminate early on all Cortex-M cores that
 *    implement them, so these are omitted.
 */
#ifndef CONFIG_CPU_CORTEX_M33
#error "ML-DSA is currently supported on Cortex-M33 devices only."
#endif

/* FIPS 204, Section 4, global parameters (shared by all parameter sets). */

/* n: the number of coefficients in a polynomial vector */
#define ML_DSA_POLY_COEFFS_COUNT  256
/* prime number q = 2^23 - 2^13 + 1 */
#define ML_DSA_PRIME_NUM	  8380417
/* d: number of dropped bits from each coefficient of the vector t */
#define ML_DSA_DROPPED_BITS_COUNT 13
/* zeta: a primitive 512-th root of unity mod q */
#define ML_DSA_ROOT_OF_UNITY	  1753
/* gamma2 = (q - 1) / val */
#define ML_DSA_GAMMA2(val)	  ((ML_DSA_PRIME_NUM - 1) / val)

/* Maximum matrix/vector dimensions based on required key size.
 * According to algorithm notation, rows count is "k" and columns is "l".
 *
 * !!! ORDER MATTERS !!!
 */
#if defined(PSA_NEED_CRACEN_ML_DSA_87)
#define ML_DSA_MATRIX_ROWS_MAX 8
#define ML_DSA_MATRIX_COLS_MAX 7
#elif defined(PSA_NEED_CRACEN_ML_DSA_65)
#define ML_DSA_MATRIX_ROWS_MAX 6
#define ML_DSA_MATRIX_COLS_MAX 5
#elif defined(PSA_NEED_CRACEN_ML_DSA_44)
#define ML_DSA_MATRIX_ROWS_MAX 4
#define ML_DSA_MATRIX_COLS_MAX 4
#else
/* A static assert is used in the cracen_ml_dsa.c file against size 1. */
#define ML_DSA_MATRIX_ROWS_MAX 1
#define ML_DSA_MATRIX_COLS_MAX 1
#endif

/* Buffer sizes. */
#define ML_DSA_SEED_SZ_BYTES		 32
#define ML_DSA_KEY_PAIR_SEED_SZ_BYTES	 32  /* xi: KeyGen seed / PSA key-pair import format */
#define ML_DSA_K_SZ_BYTES		 32  /* K: signing secret from key expansion */
#define ML_DSA_RND_SZ_BYTES		 32  /* rnd: per-signature randomizer */
#define ML_DSA_RHO_PRIME_SZ_BYTES	 64  /* rho': ExpandS seed from key expansion */
#define ML_DSA_KEYGEN_H_OUT_SZ_BYTES	 128 /* rho(32) | rho'(64) | K(32); FIPS 204, Algorithm 6 */
#define ML_DSA_PK_DIGEST_SZ_BYTES	 64
#define ML_DSA_MSG_RPZTV_SZ_BYTES	 64
#define ML_DSA_REJ_NTT_SEED_BYTES	 (ML_DSA_SEED_SZ_BYTES + 2)
#define ML_DSA_PRIV_SEED_BYTES		 64
#define ML_DSA_REJ_BOUNDED_SEED_BYTES	 (ML_DSA_RHO_PRIME_SZ_BYTES + 2)
#define ML_DSA_EXPAND_MASK_SEED_BYTES	 (ML_DSA_RHO_PRIME_SZ_BYTES + 2)
#define ML_DSA_T1_PACKED_POLY_BYTES	 320 /* SimpleBitPack of one t1 poly (10 bits) */
#define ML_DSA_C_TILDE_MAX_BYTES	 64 /* 2*lambda/8 for ML-DSA-87 */
#define ML_DSA_COMMITMENT_SIZE_MAX_BYTES (ML_DSA_MATRIX_ROWS_MAX * \
					  ((ML_DSA_POLY_COEFFS_COUNT * 6) / 8))
/* Largest encoded public key pkEncode(rho, t1) = rho(32) || SimpleBitPack(t1) per row.
 * Maximized by ML-DSA-87 (k = 8): 32 + 8 * 320 = 2592 bytes.
 */
#define ML_DSA_PK_SIZE_MAX_BYTES	 (ML_DSA_SEED_SZ_BYTES + \
					  ML_DSA_MATRIX_ROWS_MAX * ML_DSA_T1_PACKED_POLY_BYTES)

/* Largest ExpandMask squeeze: 32 * (1 + bitlen(gamma1 - 1)) bytes, maximized by
 * gamma1 = 2^19 (ML-DSA-65/87), giving 32 * 20 = 640 bytes.
 */
#define ML_DSA_EXPAND_MASK_MAX_BYTES	 (32 * 20)

/** SimpleBitUnpack range for t1 (FIPS 204, Alg 22 and 23):
 *  2^(bitlen(q-1) - d) - 1 = 2^10 - 1
 */
#define ML_DSA_T1_COEFF_MAX		 ((1u << (23 - ML_DSA_DROPPED_BITS_COUNT)) - 1u)

/* DER object identifiers of the pre-hash functions, as used by HashML-DSA
 * (FIPS 204, Algorithm 5). Each is a complete DER OID (tag 0x06, length 0x09).
 */
#define ML_DSA_HASH_OID_BYTES		 11

#define ML_DSA_MAX_CONTEXT_LENGTH	 255

#define ML_DSA_SIGNERS_COMMITMENT_HASH_SZ(lambda) (2 * lambda / 8)

/** @brief A polynomial vector, degree-255.
 *
 * Coefficients are kept in Z_q (the range is [0, ML_DSA_PRIME_NUM)).
 */
struct ml_dsa_poly_vector_s {
	int32_t coeffs[ML_DSA_POLY_COEFFS_COUNT];
};
typedef struct ml_dsa_poly_vector_s ml_dsa_poly_vector_t;

/** @brief Per-parameter-set constants (FIPS 204, Section 4, Table 1 and Table 2). */
struct ml_dsa_params_s {
	uint8_t rows_k;		/* rows of A / size of t1, w, h vectors */
	uint8_t columns_l;	/* columns of A / size of z vector */
	uint8_t tau;		/* Hamming weight (# of +-1 coefficients in the challenge c) */
	uint32_t (*coeff_from_half_byte)(uint8_t b, int32_t *out); /* FIPS 204, Algorithm 15 */
	uint16_t lambda;	/* collision strength (also the PSA key bits) */
	uint32_t beta;		/* beta = tau * eta */
	uint32_t gamma1;	/* coefficient range of signer's response z */
	uint32_t gamma2;	/* low-order rounding range */
	uint16_t omega;		/* max number of 1's in the hint h */
	uint8_t w1_max;		/* max w1 coefficient */
	size_t priv_key_size;	/* private key size */
	size_t pk_size;		/* encoded public key size */
	size_t sig_size;	/* encoded signature size */
};
typedef struct ml_dsa_params_s ml_dsa_params_t;

/** @brief Look up the parameter set for a PSA key-bits value.
 *
 * @param[in] bits  PSA key bits: 128 -> ML-DSA-44, 192 -> ML-DSA-65,
 *                  256 -> ML-DSA-87.
 *
 * @return Pointer to the matching parameter set, or NULL if unsupported or not
 *         enabled in the build.
 */
const ml_dsa_params_t *cracen_ml_dsa_params_get(size_t bits);

/** @brief Look up the DER OID for a PSA hash algorithm, for use in HashML-DSA.
 *
 * @param[in] alg  PSA algorithm identifier.
 *
 * @return Pointer to a static ML_DSA_HASH_OID_BYTES-byte DER OID, or NULL if
 *         the hash is not supported.
 */
const uint8_t *cracen_ml_dsa_hash_oid(psa_algorithm_t alg);

/** @brief Compute the packed byte size of a polynomial vector.
 *
 * @param[in] bit_len  Number of bits used to encode each coefficient.
 *
 * @return Size in bytes of ML_DSA_POLY_COEFFS_COUNT coefficients packed at
 *         @p bit_len bits each.
 */
size_t cracen_ml_dsa_calc_vector_sz_bytes(uint32_t bit_len);

/** @brief Compute the bit length of a positive integer x
 *         (see bitlen - FIPS 204, Section 2.3).
 *
 * @param[in] x  Positive integer.
 *
 * @return The number of digits that would appear in a base-2 representation of x,
 *         where the most significant digit in the representation is assumed to be a 1.
 */
uint32_t cracen_ml_dsa_bit_length(uint32_t x);

/** @brief Reduce a coefficient into the range [0, q).
 *
 * @param[in] a  Coefficient, assumed to lie in (-q, 2q).
 *
 * @return a mod q, in [0, ML_DSA_PRIME_NUM).
 */
int32_t cracen_ml_dsa_reduce_to_zq(int32_t a);

/** @brief Calculate absolute value of a signed coefficient.
 *
 * @note This function can be used in constant time operations.
 *
 * @param[in] coeff  Coefficient.
 *
 * @return Absolute value of a signed coefficient.
 */
int32_t cracen_ml_dsa_abs_coeff(int32_t coeff);

/** @brief Get a mask value indicating if the value is greater or equal
 *         than specified bound (magnitude >= bound).
 *
 * @note This function can be used in constant time operations.
 *
 * @param[in] magnitude  Absolute value of a coefficient.
 * @param[in] bound      Bound the magnitude is compared against.
 *
 * @return All-ones mask when @p magnitude >= @p bound, zero otherwise.
 */
int32_t cracen_ml_dsa_ge_bound_mask(int32_t magnitude, int32_t bound);

/** @brief Compute the SHAKE256 digest of a single buffer.
 *
 * @param[in] in       Input buffer.
 * @param[in] in_len   Length of @p in, in bytes.
 * @param[out] out     Output buffer.
 * @param[in] out_len  Number of digest bytes to produce.
 *
 * @return PSA_SUCCESS, or a PSA error code otherwise.
 */
psa_status_t cracen_ml_dsa_shake256_digest(const uint8_t *in, size_t in_len,
					   uint8_t *out, size_t out_len);

/** @brief Compute mu = H(tr || M'), the message representative used in
 *         signing and verification.
 *
 * M' is the formatted message of FIPS 204, Section 5.2:
 * M' = IntegerToBytes(domain, 1) || IntegerToBytes(|ctx|, 1) || ctx [|| oid] || msg.
 * For HashML-DSA (domain 1), @p oid is the pre-hash OID and @p msg is PH(M).
 *
 * @param[in] pk_digest           tr: the ML_DSA_PK_DIGEST_SZ_BYTES-byte public key digest.
 * @param[in] domain              0 for pure ML-DSA, 1 for HashML-DSA.
 * @param[in] ctx                 Context string.
 * @param[in] ctx_len             Length of @p ctx, in bytes (at most ML_DSA_MAX_CONTEXT_LENGTH).
 * @param[in] oid                 Pre-hash function DER OID (HashML-DSA only), may be NULL for
 *                                pure ML-DSA.
 * @param[in] oid_len             Length of @p oid, in bytes (0 for pure ML-DSA).
 * @param[in] msg                 Message, or the pre-hashed message for HashML-DSA.
 * @param[in] msg_len             Length of @p msg, in bytes.
 * @param[out] msg_representative Output buffer of ML_DSA_MSG_RPZTV_SZ_BYTES bytes for mu.
 *
 * @return PSA_SUCCESS, or a PSA error code otherwise.
 */
psa_status_t cracen_ml_dsa_compute_msg_representative(const uint8_t *pk_digest, uint8_t domain,
						      const uint8_t *ctx, size_t ctx_len,
						      const uint8_t *oid, size_t oid_len,
						      const uint8_t *msg, size_t msg_len,
						      uint8_t *msg_representative);

/** @brief Compute c_tilde = H(mu || w1Encode(w1)), the commitment hash used to
 *         derive the challenge polynomial c during signing, and to verify a
 *         signature (FIPS 204, Algorithms 7 and 8).
 *
 * @param[in] msg_representative   mu: the ML_DSA_MSG_RPZTV_SZ_BYTES-byte message representative.
 * @param[in] commitment           The encoded commitment, w1Encode(w1).
 * @param[in] commitment_len       Length of @p commitment, in bytes.
 * @param[out] commitment_hash     Output buffer for c_tilde.
 * @param[in] commitment_hash_len  Number of c_tilde bytes to produce (2 * lambda / 8).
 *
 * @return PSA_SUCCESS, or a PSA error code otherwise.
 */
psa_status_t cracen_ml_dsa_compute_commitment_hash(const uint8_t *msg_representative,
						   const uint8_t *commitment,
						   size_t commitment_len,
						   uint8_t *commitment_hash,
						   size_t commitment_hash_len);

#endif /* CRACEN_ML_DSA_INTERNAL_H */
