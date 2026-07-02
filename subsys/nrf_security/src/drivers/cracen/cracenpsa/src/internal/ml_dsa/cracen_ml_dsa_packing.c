/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"
#include "cracen_ml_dsa_packing.h"

#include <nrf_security_mem_helpers.h>

/*
 * Bit (un)packing follows the conventions of FIPS 204, Section 7.1: bits are
 * ordered least-significant-first within each byte (BytesToBits / BitsToBytes)
 * and integers are encoded little-endian (IntegerToBits / BitsToInteger).
 */

struct bit_reader {
	const uint8_t *buf;
	size_t bit_pos;
};

struct bit_writer {
	uint8_t *buf;
	size_t bit_pos;
};

/* Read @p n bits (n <= 32) as a little-endian unsigned integer. */
static uint32_t read_bits(struct bit_reader *r, uint32_t n)
{
	uint32_t val;

	val = 0;
	for (uint32_t i = 0; i < n; i++) {
		size_t byte = r->bit_pos >> 3;
		uint32_t bit = (uint32_t)(r->bit_pos & 7u);
		uint32_t b = (uint32_t)((r->buf[byte] >> bit) & 1u);

		val |= b << i;
		r->bit_pos++;
	}

	return val;
}

/* Write the low @p n bits of @p val (n <= 32), little-endian. */
static void write_bits(struct bit_writer *w, uint32_t val, uint32_t n)
{
	for (uint32_t i = 0; i < n; i++) {
		size_t byte = w->bit_pos >> 3;
		uint32_t bit = (uint32_t)(w->bit_pos & 7u);

		if (bit == 0) {
			w->buf[byte] = 0;
		}
		w->buf[byte] |= (uint8_t)(((val >> i) & 1u) << bit);
		w->bit_pos++;
	}
}

void cracen_ml_dsa_simple_bit_pack(const ml_dsa_poly_vector_t *w, uint32_t b,
				   uint8_t *out)
{
	uint32_t c = cracen_ml_dsa_bit_length(b);
	struct bit_writer wr = {.buf = out, .bit_pos = 0};

	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		write_bits(&wr, (uint32_t)w->coeffs[i], c);
	}
}

void cracen_ml_dsa_simple_bit_unpack(const uint8_t *v, uint32_t b,
				     ml_dsa_poly_vector_t *out_vec)
{
	uint32_t c = cracen_ml_dsa_bit_length(b);
	struct bit_reader r = {.buf = v, .bit_pos = 0};

	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		out_vec->coeffs[i] = (int32_t)read_bits(&r, c);
	}
}

void cracen_ml_dsa_bit_unpack(const uint8_t *v, uint32_t a, uint32_t b,
			      ml_dsa_poly_vector_t *out_vec)
{
	uint32_t c = cracen_ml_dsa_bit_length(a + b);
	struct bit_reader r = {.buf = v, .bit_pos = 0};

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
