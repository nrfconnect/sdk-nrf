/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_kem_internal.h"
#include "cracen_ml_kem_sampling.h"

#include <internal/pqc/cracen_pqc_bits.h>
#include <internal/pqc/cracen_pqc_xof.h>
#include <cracen_psa_primitives.h>
#include <cracen_psa_xof.h>
#include <nrf_security_mem_helpers.h>
#include <string.h>
#include <zephyr/sys/util.h>

#define ML_KEM_XOF_BLOCK_SZ_BYTES	CONFIG_CRACEN_XOF_OUT_POOL_BUF_SIZE

/* The number of bytes reqired by a single iteration of the rejection loop. */
#define ML_KEM_XOF_BYTES_PER_SQUEEZE	3

/* Largest eta of any parameter set, bounding the PRF output (64 * eta bytes). */
#define ML_KEM_ETA_MAX			3

/* FIPS203, Section 4.1: PRF_eta returns (64*eta)-byte output. */
#define ML_KEM_MAX_PRF_OUT_SZ_BYTES	(64u * ML_KEM_ETA_MAX)

/* The XOF seed of SampleNTT: rho || i || j */
#define ML_KEM_XOF_SEED_SZ_BYTES	(ML_KEM_SEED_HALF_SZ_BYTES + 2)

/* The PRF seed of SamplePolyCBD: sigma || nonce */
#define ML_KEM_PRF_SEED_SZ_BYTES	(ML_KEM_SEED_HALF_SZ_BYTES + 1)

psa_status_t cracen_ml_kem_sample_ntt(const uint8_t *rho, uint8_t index0, uint8_t index1,
				      ml_kem_poly_t *out)
{
	cracen_xof_operation_t operation;
	psa_status_t status;
	uint8_t seed[ML_KEM_XOF_SEED_SZ_BYTES];
	uint8_t bytes[ML_KEM_XOF_BYTES_PER_SQUEEZE * ML_KEM_XOF_BLOCK_SZ_BYTES];
	size_t available = sizeof(bytes);
	size_t pos = 0;
	uint32_t j = 0;

	memcpy(seed, rho, ML_KEM_SEED_HALF_SZ_BYTES);
	seed[ML_KEM_SEED_HALF_SZ_BYTES] = index0;
	seed[ML_KEM_SEED_HALF_SZ_BYTES + 1] = index1;

	status = cracen_xof_setup(&operation, PSA_ALG_SHAKE128);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_xof_update(&operation, seed, sizeof(seed));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_xof_output(&operation, bytes, available);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	while (j < ML_KEM_POLY_COEFFS_COUNT) {
		/* FIPS 203, Algorithm 7: three bytes carry two 12-bit candidates. */
		uint32_t d1;
		uint32_t d2;

		if (pos == available) {
			/* Rejections exhausted the buffer: squeeze one more block. */
			available = sizeof(bytes);
			status = cracen_xof_output(&operation, bytes, available);
			if (status != PSA_SUCCESS) {
				goto exit;
			}
			pos = 0;
		}

		d1 = bytes[pos] + 256u * ((uint32_t)bytes[pos + 1] & 0x0Fu);
		d2 = ((uint32_t)bytes[pos + 1] >> 4) + 16u * bytes[pos + 2];
		pos += ML_KEM_XOF_BYTES_PER_SQUEEZE;

		if (d1 < ML_KEM_PRIME_NUM) {
			out->coeffs[j] = (int16_t)d1;
			j++;
		}

		if (d2 < ML_KEM_PRIME_NUM && j < ML_KEM_POLY_COEFFS_COUNT) {
			out->coeffs[j] = (int16_t)d2;
			j++;
		}
	}

exit:
	safe_memzero(bytes, sizeof(bytes));
	(void)cracen_xof_abort(&operation);
	return status;
}

psa_status_t cracen_ml_kem_sample_poly_cbd(const uint8_t *seed, uint8_t nonce, uint8_t eta,
					   ml_kem_poly_t *out)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t prf_seed[ML_KEM_PRF_SEED_SZ_BYTES];
	uint8_t prf_out_bytes[ML_KEM_MAX_PRF_OUT_SZ_BYTES];
	size_t prf_out_bytes_len = 64u * eta;

	if (eta > ML_KEM_ETA_MAX) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	memcpy(prf_seed, seed, ML_KEM_SEED_HALF_SZ_BYTES);
	prf_seed[ML_KEM_SEED_HALF_SZ_BYTES] = nonce;

	const uint8_t *const prf_chunks[] = {prf_seed};
	const size_t prf_chunk_lengths[]  = {sizeof(prf_seed)};

	status = cracen_pqc_xof_compute(PSA_ALG_SHAKE256,
					prf_chunks, prf_chunk_lengths, ARRAY_SIZE(prf_chunks),
					prf_out_bytes, prf_out_bytes_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	struct pqc_bit_reader reader = {.next = prf_out_bytes, .acc = 0, .acc_bits = 0};

	for (uint32_t i = 0; i < ML_KEM_POLY_COEFFS_COUNT; i++) {
		int16_t x = 0;
		int16_t y = 0;

		/** FIPS 203, Algorithm 8: two sums of eta consecutive bits each. Their
		 *  difference lies in [-eta, eta], which is already a signed
		 *  representative of (x - y) mod q; the canonical form is produced when
		 *  the coefficient is encoded or compressed (e.g. by calling cracen_ml_kem_ntt()).
		 */
		for (uint8_t j = 0; j < eta; j++) {
			x += (int16_t)cracen_pqc_read_bits(&reader, 1);
		}

		for (uint8_t j = 0; j < eta; j++) {
			y += (int16_t)cracen_pqc_read_bits(&reader, 1);
		}

		out->coeffs[i] = (int16_t)(x - y);
	}

exit:
	safe_memzero(prf_out_bytes, sizeof(prf_out_bytes));
	safe_memzero(prf_seed, sizeof(prf_seed));
	return status;
}
