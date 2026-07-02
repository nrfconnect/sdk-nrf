/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal definitions for the CRACEN ML-DSA (FIPS 204) verifier.
 *
 * Shared types, global constants and the prototypes of the internal helpers.
 * Helper functions are named after the FIPS 204 algorithm they implement so
 * that the code can be checked against the specification directly. Not part of
 * the public driver API.
 */

#ifndef CRACEN_ML_DSA_INTERNAL_H
#define CRACEN_ML_DSA_INTERNAL_H

#include <psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* FIPS 204, Section 4, global parameters (shared by all parameter sets). */

/* n: the number of coefficients in a polynomial vector */
#define ML_DSA_POLY_COEFFS_COUNT  256
/* prime number q = 2^23 - 2^13 + 1 */
#define ML_DSA_PRIME_NUM	  8380417
/* d: number of dropped bits from each coefficient of the vector t */
#define ML_DSA_DROPPED_BITS_COUNT 13
/* zeta: a primitive 512-th root of unity mod q */
#define ML_DSA_ROOT_OF_UNITY	  1753

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
/* A static assert is used in the cracen_ml_dsa.c against size 1. */
#define ML_DSA_MATRIX_ROWS_MAX 1
#define ML_DSA_MATRIX_COLS_MAX 1
#endif

/* Buffer sizes. */
#define ML_DSA_SEED_SZ_BYTES		 32
#define ML_DSA_PK_DIGEST_SZ_BYTES	 64
#define ML_DSA_MSG_RPZTV_SZ_BYTES	 64
#define ML_DSA_REJ_NTT_SEED_BYTES	 (ML_DSA_SEED_SZ_BYTES + 2)
#define ML_DSA_T1_PACKED_POLY_BYTES	 320 /* SimpleBitPack of one t1 poly (10 bits) */
#define ML_DSA_C_TILDE_MAX_BYTES	 64 /* 2*lambda/8 for ML-DSA-87 */
#define ML_DSA_COMMITMENT_SIZE_MAX_BYTES (ML_DSA_MATRIX_ROWS_MAX * \
					  ((ML_DSA_POLY_COEFFS_COUNT * 6) / 8))

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

/**
 * @brief Computes the bit length of a positive integer x
 *	  (see bitlen - FIPS 204, Section 2.3).
 *
 * @param[in] x Positive integer.
 *
 * @return The number of digits that would appear in a base-2 representation of x,
 *	   where the most significant digit in the representation is assumed to be a 1.
 */
uint32_t cracen_ml_dsa_bit_length(uint32_t x);

#endif /* CRACEN_ML_DSA_INTERNAL_H */
