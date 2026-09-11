/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"
#include "cracen_ml_dsa_packing.h"

#include <internal/pqc/cracen_pqc_bits.h>
#include <nrf_security_mem_helpers.h>

/*
 * Bit packing and unpacking follows the conventions of FIPS 204, Section 7.1: bits are
 * ordered least-significant-first within each byte (BytesToBits / BitsToBytes)
 * and integers are encoded little-endian (IntegerToBits / BitsToInteger).
 */

void cracen_ml_dsa_simple_bit_pack(const ml_dsa_poly_vector_t *w, uint32_t b,
				   uint8_t *out)
{
	uint32_t c = cracen_pqc_bit_length(b);
	struct pqc_bit_writer wr = {.next = out, .acc = 0, .acc_bits = 0};

	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		cracen_pqc_write_bits(&wr, (uint32_t)w->coeffs[i], c);
	}
}

void cracen_ml_dsa_simple_bit_unpack(const uint8_t *v, uint32_t b,
				     ml_dsa_poly_vector_t *out_vec)
{
	/* v - array of 320 bytes (32 * 10 bits); b - upper bound of single coefficient */
	uint32_t c = cracen_pqc_bit_length(b);
	struct pqc_bit_reader r = {.next = v, .acc = 0, .acc_bits = 0};

	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		out_vec->coeffs[i] = (int32_t)cracen_pqc_read_bits(&r, c);
	}
}

void cracen_ml_dsa_bit_unpack(const uint8_t *v, uint32_t a, uint32_t b,
			      ml_dsa_poly_vector_t *out_vec)
{
	uint32_t c = cracen_pqc_bit_length(a + b);
	struct pqc_bit_reader r = {.next = v, .acc = 0, .acc_bits = 0};

	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		out_vec->coeffs[i] = (int32_t)b - (int32_t)cracen_pqc_read_bits(&r, c);
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
