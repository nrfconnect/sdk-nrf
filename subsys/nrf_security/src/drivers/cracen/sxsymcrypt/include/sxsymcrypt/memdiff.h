/**
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MEMDIFF_HEADER_FILE
#define MEMDIFF_HEADER_FILE

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int sx_memdiff(const uint8_t *a, const uint8_t *b, size_t sz)
{
	int r = 0;
	size_t i;

	for (i = 0; i < sz; i++, a++, b++) {
		r |= *a ^ *b;
	}

	return r;
}

#ifdef __cplusplus
}
#endif

#endif
