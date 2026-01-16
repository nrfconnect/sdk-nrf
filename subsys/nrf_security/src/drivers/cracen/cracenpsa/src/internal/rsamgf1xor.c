/* MGF1 mask computation and bitwise xor, based on RFC 8017.
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the MGF1XOR function:
 *      1. MGF1 counter (size: MGF1_COUNTER_SZ)
 *      2. MGF1 output segment (size: digestsz).
 */

#include <sxsymcrypt/hash.h>
#include <cracen/statuscodes.h>
#include "rsamgf1xor.h"
#include "common.h"

/* number of bytes used for the MGF1 internal counter */
#define MGF1_COUNTER_SZ 4

static int mgf1xor_hash(uint8_t *mgfctr, size_t workmemsz, const struct sxhashalg *hashalg,
			uint8_t *seed, size_t digestsz)
{
	uint8_t *mask_segment = mgfctr + MGF1_COUNTER_SZ;
	uint8_t const *hash_array[] = {seed, mgfctr};
	size_t hash_array_lengths[] = {digestsz, MGF1_COUNTER_SZ};
	size_t input_count = 2;

	return cracen_hash_all_inputs(hash_array, hash_array_lengths, input_count, hashalg,
				      mask_segment);
}

int cracen_run_mgf1xor(uint8_t *workmem, size_t workmemsz, const struct sxhashalg *hashalg,
		       uint8_t *seed, size_t digestsz, uint8_t *xorinout, size_t masksz)
{

	if (workmemsz < MGF1_COUNTER_SZ + sx_hash_get_alg_digestsz(hashalg)) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	uint8_t *mgfctr = workmem;
	int sx_status;

	/* initialize the MGF1 counter to zero */
	mgfctr[0] = 0;
	mgfctr[1] = 0;
	mgfctr[2] = 0;
	mgfctr[3] = 0;

	sx_status = mgf1xor_hash(workmem, workmemsz, hashalg, seed, digestsz);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	size_t toxor;
	uint8_t *mask_segment = workmem + MGF1_COUNTER_SZ;

	/* this function: (1) computes the xor of the mask segment that was just
	 * produced with the bytes pointed by xorinout, (2) increments the MGF1 counter
	 */

	while (masksz > 0) {
		toxor = (masksz > digestsz) ? digestsz : masksz;

		cracen_xorbytes(xorinout, mask_segment, toxor);

		xorinout += toxor;
		/* masksz holds the number of remaining mask bytes to produce */
		masksz -= toxor;

		cracen_be_add(mgfctr, MGF1_COUNTER_SZ, 1);
		sx_status = mgf1xor_hash(mgfctr, workmemsz, hashalg, seed, digestsz);
		if (sx_status != SX_OK) {
			return sx_status;
		}
	}
	return SX_OK;
}
