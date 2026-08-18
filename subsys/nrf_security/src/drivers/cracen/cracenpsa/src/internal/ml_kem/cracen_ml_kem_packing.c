/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_kem_internal.h"
#include "cracen_ml_kem_packing.h"

#include <internal/pqc/cracen_pqc_bits.h>

/* bitlen(q - 1). */
#define ML_KEM_POLY_COEFF_BITS 12

/** Division by q, expressed as a multiplication by ceil(2^33 / q) followed by a
 *  shift, inspired by way mlkem-native implementation for Compress_d, where
 *  d = 11 (which is the maximum possible value).
 *
 *  A hardware divide instruction is not used on purpose: UDIV on Cortex-M33
 *  terminates early, so this might violate constant time execution requirement.
 */
#define ML_KEM_Q_RECIPROCAL	  2580335u /* round(2^33 / ML_KEM_PRIME_NUM) */
#define ML_KEM_Q_RECIPROCAL_SHIFT 33
#define ML_KEM_HALF_PRIMENUM	  1664 /* ML_KEM_PRIME_NUM / 2 */

/* Maps a coefficient in (-q, q) to its representative in [0, q). */
static uint16_t to_positive(int16_t coeff)
{
	return (uint16_t)(coeff + (ML_KEM_PRIME_NUM & (coeff >> 15)));
}

/* Compress_d: round(2^d / q * x) mod 2^d, round to the nearest integer. */
static uint32_t compress(int16_t coeff, uint8_t d)
{
	uint64_t numerator = ((uint64_t)to_positive(coeff) << d) + ML_KEM_HALF_PRIMENUM;
	uint32_t quotient =
		(uint32_t)((numerator * ML_KEM_Q_RECIPROCAL) >> ML_KEM_Q_RECIPROCAL_SHIFT);

	return quotient & ((1u << d) - 1u);
}

/* Decompress_d: round(q / 2^d * y), round to the nearest integer. */
static int16_t decompress(uint32_t y, uint8_t d)
{
	return (int16_t)((y * ML_KEM_PRIME_NUM + (1u << (d - 1))) >> d);
}

void cracen_ml_kem_poly_pack(const ml_kem_poly_t *poly, uint8_t *out)
{
	struct pqc_bit_writer w = {.next = out, .acc = 0, .acc_bits = 0};

	for (uint32_t i = 0; i < ML_KEM_POLY_COEFFS_COUNT; i++) {
		cracen_pqc_write_bits(&w, to_positive(poly->coeffs[i]), ML_KEM_POLY_COEFF_BITS);
	}
}

void cracen_ml_kem_poly_unpack(const uint8_t *in, ml_kem_poly_t *poly)
{
	struct pqc_bit_reader r = {.next = in, .acc = 0, .acc_bits = 0};

	for (uint32_t i = 0; i < ML_KEM_POLY_COEFFS_COUNT; i++) {
		poly->coeffs[i] = (int16_t)cracen_pqc_read_bits(&r, ML_KEM_POLY_COEFF_BITS);
	}
}

void cracen_ml_kem_poly_compress_encode(const ml_kem_poly_t *poly, uint8_t d, uint8_t *out)
{
	struct pqc_bit_writer w = {.next = out, .acc = 0, .acc_bits = 0};

	for (uint32_t i = 0; i < ML_KEM_POLY_COEFFS_COUNT; i++) {
		cracen_pqc_write_bits(&w, compress(poly->coeffs[i], d), d);
	}
}

void cracen_ml_kem_poly_decode_decompress(const uint8_t *in, uint8_t d, ml_kem_poly_t *poly)
{
	struct pqc_bit_reader r = {.next = in, .acc = 0, .acc_bits = 0};

	for (uint32_t i = 0; i < ML_KEM_POLY_COEFFS_COUNT; i++) {
		poly->coeffs[i] = decompress(cracen_pqc_read_bits(&r, d), d);
	}
}
