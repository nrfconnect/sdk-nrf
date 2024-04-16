/* Generation of a random number in the range from 1 to a given n (excluded).
 * Based on FIPS 186-4 (sections B.1.2, B.2.2, B.4.2 and B.5.2).
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * High level description of the algorithm:
 *      1. if (n < 3), return error
 *      2. if n is not odd, return error
 *      3. compute byte length B of n
 *      4. compute bit mask b of the most significant non-zero byte in n
 *      5. get a random byte string s of size B
 *      6. set to zero the excess bits in s, based on b
 *      7. if (s == 0) OR (s >= n), go back to step 5
 *      8. return k = s, as a byte array of same size as the input n
 *
 * Notes:
 * - any n < 3 is rejected. In fact the task has to provide a random number k
 *   such that 1 <= k < n. If n = 1 there are no valid results. If n = 2, there
 *   is only one valid result, hence not random.
 */

#include <string.h>
#include "../include/sicrypto/sicrypto.h"
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <cracen_psa.h>
#include "rndinrange.h"
#include "waitqueue.h"
#include "final.h"
#include "util.h"

static void rndinrange_getrnd(struct sitask *t);

/* Return 1 if the given byte string contains only zeros, 0 otherwise. */
static int is_zero_bytestring(char *a, size_t sz)
{
	int acc = 0;

	for (size_t i = 0; i < sz; i++) {
		acc |= a[i];
	}

	return !acc;
}

static int rndinrange_continue(struct sitask *t, struct siwq *wq)
{
	int z, ge;
	size_t rndsz = t->params.rndinrange.rndsz;
	unsigned char *out = (unsigned char *)t->output;
	const unsigned char *const upperlimit = t->params.rndinrange.n;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* set to zero excess high-order bits in the most significant byte */
	out[0] &= t->params.rndinrange.msb_mask;

	/* if the generated number is 0 or >= n, then discard it and repeat */
	z = is_zero_bytestring((char *)out, rndsz);
	ge = si_be_cmp(out, upperlimit, rndsz, 0);
	if (z || (ge >= 0)) {
		rndinrange_getrnd(t);
		return (t->statuscode = SX_ERR_HW_PROCESSING);
	}

	/* the generated number is valid */
	return SX_OK;
}

static void rndinrange_getrnd(struct sitask *t)
{
	/* Get random octets from the PRNG in the environment. Place them
	 * directly in the user's output buffer.
	 */
	psa_status_t status = cracen_get_random(NULL, t->output, t->params.rndinrange.rndsz);

	if (status != PSA_SUCCESS) {
		si_task_mark_final(t, SX_ERR_UNKNOWN_ERROR);
		return;
	}

	si_wq_run_after(t, &t->params.rndinrange.wq, rndinrange_continue);

	t->actions.status = si_status_on_prng;
	t->actions.wait = si_wait_on_prng;
}

void si_rndinrange_create(struct sitask *t, const unsigned char *n, size_t nsz, char *out)
{
	size_t index;
	unsigned char msb_mask;

	/* error if the provided upper limit has size 0 */
	if (nsz == 0) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	/* get index of most significant non-zero byte in n */
	for (index = 0; (n[index] == 0) && (index < nsz); index++) {
		;
	}

	/* error if the provided upper limit is 0 or if it is > 0 but < 3 */
	if ((index == nsz) || ((index == nsz - 1) && (n[index] < 3))) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	/* error if the provided upper limit is not odd */
	if ((n[nsz - 1] & 0x01) == 0) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* get bit mask of the most significant non-zero byte in n */
	for (msb_mask = 0xFF; n[index] & msb_mask; msb_mask <<= 1) {
		;
	}

	msb_mask = ~msb_mask;

	t->params.rndinrange.n = n + index;
	t->params.rndinrange.rndsz = nsz - index;
	t->params.rndinrange.msb_mask = msb_mask;
	t->output = out + index;

	/* write high-order zero bytes, if any, in the output buffer */
	safe_memset(out, nsz, 0, index);

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.run = rndinrange_getrnd;
}
