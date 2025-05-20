/* Sicrypto memory utility functions.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>

int si_memdiff(const char *a, const char *b, size_t sz)
{
	const volatile char *a_v = (const volatile char *)a;
	const volatile char *b_v = (const volatile char *)b;
	int r = 0;

	for (size_t i = 0; i < sz; i++) {
		r |= a_v[i] ^ b_v[i];
	}
	return r;
}
