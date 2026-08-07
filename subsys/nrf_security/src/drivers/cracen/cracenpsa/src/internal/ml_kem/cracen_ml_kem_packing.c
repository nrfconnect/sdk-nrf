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

/* Maps a coefficient in (-q, q) to its representative in [0, q). */
static uint16_t to_positive(int16_t coeff)
{
	return (uint16_t)(coeff + (ML_KEM_PRIME_NUM & (coeff >> 15)));
}

/* Compress_d: round(2^d / q * x) mod 2^d, round to the nearest integer. */
static uint32_t compress(int16_t coeff, uint8_t d)
{
	uint32_t x = to_positive(coeff);

	return (((x << d) + ML_KEM_PRIME_NUM / 2) / ML_KEM_PRIME_NUM) & ((1u << d) - 1u);
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
