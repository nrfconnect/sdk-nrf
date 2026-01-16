/**
 * @file
 *
 * @copyright Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <cracen_psa.h>
#include "common.h"

/* Return 1 if the given byte string contains only zeros, 0 otherwise. */
static int is_zero_bytestring(const uint8_t *a, size_t sz)
{
	int acc = 0;

	for (size_t i = 0; i < sz; i++) {
		acc |= a[i];
	}
	return !acc;
}

static int rnd_in_range_continue(uint8_t *out, size_t rndsz, const uint8_t *upper_limit,
				 uint8_t msb_mask)
{
	int z;
	int ge;

	/* Set to zero excess high-order bits in the most significant byte */
	out[0] &= msb_mask;

	/* If the generated number is 0 or >= n, then discard it and repeat */
	z = is_zero_bytestring(out, rndsz);
	ge = cracen_be_cmp(out, upper_limit, rndsz, 0);
	if (z || (ge >= 0)) {
		return SX_ERR_HW_PROCESSING;
	}

	/* The generated number is valid */
	return SX_OK;
}

static int rnd_in_range_get_rnd(uint8_t *out, size_t rndsz, const uint8_t *upper_limit,
				uint8_t msb_mask)
{
	/* Get random octets from the PRNG in the environment.
	 * Place them directly in the user's output buffer.
	 */
	int sx_status;
	psa_status_t psa_status;

	do {
		psa_status = cracen_get_random(NULL, out, rndsz);

		if (psa_status != PSA_SUCCESS) {
			return SX_ERR_UNKNOWN_ERROR;
		}
		sx_status = rnd_in_range_continue(out, rndsz, upper_limit, msb_mask);

	} while (sx_status == SX_ERR_HW_PROCESSING);

	return SX_OK;
}

int cracen_get_rnd_in_range(const uint8_t *upper_limit, size_t upper_limit_sz, uint8_t *out)
{
	size_t index;
	uint8_t msb_mask;

	/* Error if the provided upper limit has size 0 */
	if (upper_limit_sz == 0) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* Get index of most significant non-zero byte in n */
	for (index = 0; (upper_limit[index] == 0) && (index < upper_limit_sz); index++) {
		/* Do nothing; just increment 'index' */
	}

	/* Error if the provided upper limit is 0 or if it is > 0 but < 3 */
	if ((index == upper_limit_sz) ||
	    ((index == upper_limit_sz - 1) && (upper_limit[index] < 3))) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* Error if the provided upper limit is not odd */
	if ((upper_limit[upper_limit_sz - 1] & 0x01) == 0) {
		return SX_ERR_INVALID_ARG;
	}

	/* Get bit mask of the most significant non-zero byte in n */
	msb_mask = 0xFF;
	while (upper_limit[index] & msb_mask) {
		msb_mask <<= 1;
	}
	msb_mask = ~msb_mask;

	size_t rndsz = upper_limit_sz - index;
	const uint8_t *adjusted_upper_limit = upper_limit + index;
	uint8_t *adjusted_out = out + index;

	/* Write high-order zero bytes, if any, in the output buffer */
	safe_memset(out, upper_limit_sz, 0, index);
	return rnd_in_range_get_rnd(adjusted_out, rndsz, adjusted_upper_limit, msb_mask);
}
