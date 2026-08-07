/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal definitions for the CRACEN ML-KEM (FIPS 203) implementation.
 *
 * Shared types and per-parameter-set constants. Not part of the public driver
 * API.
 */

#ifndef CRACEN_ML_KEM_INTERNAL_H
#define CRACEN_ML_KEM_INTERNAL_H

#include <psa/crypto_values.h>
#include <stddef.h>
#include <stdint.h>

/* FIPS 203, Section 4, global parameters (shared by all parameter sets). */

/* n: the number of coefficients in a polynomial vector */
#define ML_KEM_POLY_COEFFS_COUNT 256
/* prime number q = 2^8 * 13 + 1 */
#define ML_KEM_PRIME_NUM	 3329

#define ML_KEM_SEED_SZ_BYTES	      64  /* The (d || z) seed pair size. */
#define ML_KEM_SEED_HALF_SZ_BYTES     (ML_KEM_SEED_SZ_BYTES / 2)
#define ML_KEM_MSG_SZ_BYTES	      32
#define ML_KEM_SHARED_SECRET_SZ_BYTES 32
#define ML_KEM_PK_DIGEST_SZ_BYTES     32  /* H(ek) */
#define ML_KEM_G_OUTPUT_SZ_BYTES      64  /* G returns two 32-byte values */
#define ML_KEM_POLY_PACKED_SZ_BYTES   384 /* ByteEncode_12 of one polynomial: 256 * 12 / 8 */

/* Maximum matrix dimension k based on the enabled parameter sets.
 *
 * !!! ORDER MATTERS !!!
 */
#if defined(PSA_NEED_CRACEN_ML_KEM_1024)
#define ML_KEM_MATRIX_DIM_MAX 4
#elif defined(PSA_NEED_CRACEN_ML_KEM_768)
#define ML_KEM_MATRIX_DIM_MAX 3
#elif defined(PSA_NEED_CRACEN_ML_KEM_512)
#define ML_KEM_MATRIX_DIM_MAX 2
#else
/* A static assert is used in cracen_ml_kem.c file against size 1. */
#define ML_KEM_MATRIX_DIM_MAX 1
#endif

/* Largest encapsulation key and ciphertext of the enabled parameter sets
 * (FIPS 203, Table 3), used to size the buffers of the operations that have to
 * regenerate them.
 *
 * !!! ORDER MATTERS !!!
 */
#if defined(PSA_NEED_CRACEN_ML_KEM_1024)
#define ML_KEM_PK_MAX_SZ_BYTES	       1568
#define ML_KEM_CIPHERTEXT_MAX_SZ_BYTES 1568
#elif defined(PSA_NEED_CRACEN_ML_KEM_768)
#define ML_KEM_PK_MAX_SZ_BYTES	       1184
#define ML_KEM_CIPHERTEXT_MAX_SZ_BYTES 1088
#else
#define ML_KEM_PK_MAX_SZ_BYTES	       800
#define ML_KEM_CIPHERTEXT_MAX_SZ_BYTES 768
#endif

/** @brief A polynomial in Z_q[X]/(X^256 + 1), degree-255.
 *
 * Coefficients are signed and, unless a function documents otherwise, kept
 * reduced to the range (-ML_KEM_PRIME_NUM, ML_KEM_PRIME_NUM).
 */
struct ml_kem_poly_s {
	int16_t coeffs[ML_KEM_POLY_COEFFS_COUNT];
};
typedef struct ml_kem_poly_s ml_kem_poly_t;
typedef ml_kem_poly_t ml_kem_poly_vec_t[ML_KEM_MATRIX_DIM_MAX];

/** @brief Per-parameter-set constants (FIPS 203, Section 8, Table 2). */
struct ml_kem_params_s {
	uint8_t k;		/* matrix dimension: 2 -> 512, 3 -> 768, 4 -> 1024 */
	uint8_t eta1;		/* CBD parameter of the secret and the noise vectors */
	uint8_t eta2;		/* CBD parameter of the encryption noise */
	uint8_t du;		/* bits per coefficient of the compressed u vector */
	uint8_t dv;		/* bits per coefficient of the compressed v polynomial */
	uint16_t key_bits;	/* PSA key bits (also the parameter-set number) */
	size_t pk_size;		/* encoded encapsulation key (ek) size */
	size_t dk_size;		/* encoded decapsulation key (dk) size */
	size_t ciphertext_size; /* encoded ciphertext size */
};
typedef struct ml_kem_params_s ml_kem_params_t;

/** @brief Look up the parameter set for a PSA key-bits value.
 *
 * @param[in] bits  PSA key bits: 512 -> ML-KEM-512, 768 -> ML-KEM-768,
 *                  1024 -> ML-KEM-1024.
 *
 * @return Pointer to the matching parameter set, or NULL if unsupported or not
 *         enabled in the build.
 */
const ml_kem_params_t *cracen_ml_kem_params_get(size_t bits);

#endif /* CRACEN_ML_KEM_INTERNAL_H */
