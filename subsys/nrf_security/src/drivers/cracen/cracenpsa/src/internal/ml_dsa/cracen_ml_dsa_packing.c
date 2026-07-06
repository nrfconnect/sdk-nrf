/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"
#include "cracen_ml_dsa_packing.h"

#include <cracen/common.h>
#include <nrf_security_mem_helpers.h>

/*
 * Bit packing and unpacking follows the conventions of FIPS 204, Section 7.1: bits are
 * ordered least-significant-first within each byte (BytesToBits / BitsToBytes)
 * and integers are encoded little-endian (IntegerToBits / BitsToInteger).
 *
 * The reader and writer below stream the packed bytes through a 64-bit
 * accumulator, loading and storing one 32-bit word at a time. They rely on
 * every packed ML-DSA polynomial being a whole number of 32-bit words long
 * (256 coefficients x c bits = 32 * c bytes for every coefficient width c):
 * the reader then never loads past the end of the packed buffer, and the
 * writer's accumulator is exactly empty after the last coefficient.
 */

struct bit_reader {
	const uint8_t *next; /* next word to load */
	uint64_t acc;	     /* not yet consumed bits, LSB first */
	uint32_t acc_bits;   /* number of valid bits in acc */
};

struct bit_writer {
	uint8_t *next;	   /* next word to store */
	uint64_t acc;	   /* not yet stored bits, LSB first */
	uint32_t acc_bits; /* number of valid bits in acc */
};

/* Read @p n bits (n <= 32) as a little-endian unsigned integer. */
static uint32_t read_bits(struct bit_reader *r, uint32_t n)
{
	uint32_t val;

	if (r->acc_bits < n) {
		r->acc |= (uint64_t)cracen_get_le32(r->next) << r->acc_bits;
		r->next += sizeof(uint32_t);
		r->acc_bits += 32;
	}

	val = (uint32_t)(r->acc & (((uint64_t)1 << n) - 1u));
	r->acc >>= n;
	r->acc_bits -= n;

	return val;
}

/* Write the low @p n bits of @p val (n <= 32), little-endian. */
static void write_bits(struct bit_writer *w, uint32_t val, uint32_t n)
{
	w->acc |= ((uint64_t)val & (((uint64_t)1 << n) - 1u)) << w->acc_bits;
	w->acc_bits += n;

	if (w->acc_bits >= 32) {
		cracen_put_le32((uint32_t)w->acc, w->next);
		w->next += sizeof(uint32_t);
		w->acc >>= 32;
		w->acc_bits -= 32;
	}
}

void cracen_ml_dsa_simple_bit_pack(const ml_dsa_poly_vector_t *w, uint32_t b,
				   uint8_t *out)
{
	uint32_t c = cracen_ml_dsa_bit_length(b);
	struct bit_writer wr = {.next = out, .acc = 0, .acc_bits = 0};

	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		write_bits(&wr, (uint32_t)w->coeffs[i], c);
	}
}

void cracen_ml_dsa_simple_bit_unpack(const uint8_t *v, uint32_t b,
				     ml_dsa_poly_vector_t *out_vec)
{
	/* v - array of 320 bytes (32 * 10 bits); b - upper bound of single coefficient */
	uint32_t c = cracen_ml_dsa_bit_length(b);
	struct bit_reader r = {.next = v, .acc = 0, .acc_bits = 0};

	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		out_vec->coeffs[i] = (int32_t)read_bits(&r, c);
	}
}

void cracen_ml_dsa_bit_unpack(const uint8_t *v, uint32_t a, uint32_t b,
			      ml_dsa_poly_vector_t *out_vec)
{
	uint32_t c = cracen_ml_dsa_bit_length(a + b);
	struct bit_reader r = {.next = v, .acc = 0, .acc_bits = 0};

	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		out_vec->coeffs[i] = (int32_t)b - (int32_t)read_bits(&r, c);
	}
}

bool cracen_ml_dsa_hint_bit_unpack(const ml_dsa_params_t *alg_params, const uint8_t *y, uint8_t *h)
{
	uint32_t omega = alg_params->omega;
	uint32_t index;

	safe_memzero(h, alg_params->rows_k * ML_DSA_POLY_COEFFS_COUNT);

	index = 0;
	for (uint32_t i = 0; i < alg_params->rows_k; i++) {
		uint32_t end = y[omega + i];
		uint32_t first = index;

		if (end < index || end > omega) {
			return false;
		}

		while (index < end) {
			if (index > first && y[index - 1] >= y[index]) {
				return false;
			}
			h[i * ML_DSA_POLY_COEFFS_COUNT + y[index]] = 1;
			index++;
		}
	}

	for (uint32_t i = index; i < omega; i++) {
		if (y[i] != 0) {
			return false;
		}
	}

	return true;
}
