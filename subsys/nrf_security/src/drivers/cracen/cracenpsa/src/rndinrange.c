/**
 * @file
 *
 * @copyright Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include "../include/sicrypto/sicrypto.h"
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <cracen_psa.h>

/* Return 1 if the given byte string contains only zeros, 0 otherwise. */
static int is_zero_bytestring(const char *a, size_t sz)
{
	int acc = 0;

	for (size_t i = 0; i < sz; i++) {
		acc |= a[i];
	}
	return !acc;
}

static int rndinrange_continue(unsigned char *out, size_t rndsz, const unsigned char *upperlimit,
			       unsigned char msb_mask)
{
	int z;
	int ge;

	/* Set to zero excess high-order bits in the most significant byte */
	out[0] &= msb_mask;

	/* If the generated number is 0 or >= n, then discard it and repeat */
	z = is_zero_bytestring((char *)out, rndsz);
	ge = be_cmp(out, upperlimit, rndsz, 0);
	if (z || (ge >= 0)) {
		return SX_ERR_HW_PROCESSING;
	}

	/* The generated number is valid */
	return SX_OK;
}

static psa_status_t rndinrange_getrnd(unsigned char *out, size_t rndsz,
				      const unsigned char *upperlimit, unsigned char msb_mask)
{
	/* Get random octets from the PRNG in the environment.
	 * Place them directly in the user's output buffer.
	 */
	int result;
	psa_status_t status;

	do {
		status = cracen_get_random(NULL, out, rndsz);

		if (status != PSA_SUCCESS) {
			return SX_ERR_UNKNOWN_ERROR;
		}
		result = rndinrange_continue(out, rndsz, upperlimit, msb_mask);

	} while (result == SX_ERR_HW_PROCESSING);

	return result;
}

int rndinrange_create(const unsigned char *n, size_t nsz, unsigned char *out)
{
	size_t index;
	unsigned char msb_mask;

	/* Error if the provided upper limit has size 0 */
	if (nsz == 0) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* Get index of most significant non-zero byte in n */
	for (index = 0; (n[index] == 0) && (index < nsz); index++) {
		/* Do nothing; just increment 'index' */
	}

	/* Error if the provided upper limit is 0 or if it is > 0 but < 3 */
	if ((index == nsz) || ((index == nsz - 1) && (n[index] < 3))) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* Error if the provided upper limit is not odd */
	if ((n[nsz - 1] & 0x01) == 0) {
		return SX_ERR_INVALID_ARG;
	}

	/* Get bit mask of the most significant non-zero byte in n */
	for (msb_mask = 0xFF; n[index] & msb_mask; msb_mask <<= 1) {
		/* Do nothing; just increment 'index' */
	}
	msb_mask = ~msb_mask;

	size_t rndsz = nsz - index;
	const unsigned char *adjusted_n = n + index;
	unsigned char *adjusted_out = out + index;

	/* Write high-order zero bytes, if any, in the output buffer */
	safe_memset(out, nsz, 0, index);
	return rndinrange_getrnd(adjusted_out, rndsz, adjusted_n, msb_mask);
}
