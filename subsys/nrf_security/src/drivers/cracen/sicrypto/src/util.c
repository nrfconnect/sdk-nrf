/* Sicrypto utility functions.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <cracen/statuscodes.h>
#include "../include/sicrypto/sicrypto.h"
#include "final.h"
#include "util.h"

void si_be_add(unsigned char *v, size_t sz, size_t summand)
{
	for (; sz > 0;) {
		sz--;
		summand += v[sz];
		v[sz] = summand & 0xFF;
		summand >>= 8;
	}
}

void si_xorbytes(char *a, const char *b, size_t sz)
{
	for (size_t i = 0; i < sz; i++, a++, b++) {
		*a = *a ^ *b;
	}
}

int si_status_on_prng(struct sitask *t)
{
	/* The cracen_get_random is blocking so it always ready */
	return SX_OK;
}

int si_wait_on_prng(struct sitask *t)
{
	/* The cracen_get_random is blocking so it always ready */
	return SX_OK;
}

int si_silexpk_status(struct sitask *t)
{
	int r = sx_pk_has_finished(t->pk);

	if (r == SX_ERR_BUSY) {
		return SX_ERR_HW_PROCESSING;
	}
	return si_task_mark_final(t, r);
}

int si_silexpk_wait(struct sitask *t)
{
	int r = sx_pk_wait(t->pk);

	return si_task_mark_final(t, r);
}

int si_be_cmp(const unsigned char *a, const unsigned char *b, size_t sz, int carry)
{
	int i;
	unsigned int neq = 0, gt = 0;
	unsigned int ucarry, d, lt;

	/* transform carry to work with unsigned numbers */
	ucarry = 0x100 + carry;

	for (i = sz - 1; i >= 0; i--) {
		d = ucarry + a[i] - b[i];
		ucarry = 0xFF + (d >> 8);
		neq |= d & 0xFF;
	}

	neq |= ucarry & 0xFF;
	lt = ucarry < 0x100;
	gt = neq && !lt;

	return -lt + gt;
}
