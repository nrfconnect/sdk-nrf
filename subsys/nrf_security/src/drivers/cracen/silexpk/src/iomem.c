/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <silexpk/iomem.h>

#define PTR_ALIGNMENT(ptr, type)	(((uintptr_t)ptr) & (sizeof(type) - 1))
#define IS_NATURAL_ALIGNED(ptr, type)	!PTR_ALIGNMENT(ptr, type)
#define IS_SAME_ALIGNMENT(p1, p2, type) (PTR_ALIGNMENT(p1, type) == PTR_ALIGNMENT(p2, type))
#define MASK_TO_NATURAL_SIZE(sz, type)	((sz) & (~((sizeof(type) << 1) - 1)))
#if defined(__aarch64__)
#define IS_NATURAL_SIZE(sz, type) !((sz) & (sizeof(type) - 1))
#else
#define IS_NATURAL_SIZE(sz, type) 1
#endif

void sx_clrpkmem(void *dst, size_t sz)
{
	typedef int64_t clrblk_t;
	volatile char *d = (volatile char *)dst;

	if (sz == 0) {
		return;
	}

	while (sz && (!IS_NATURAL_ALIGNED(d, clrblk_t))) {
		*d = 0;
		d++;
		sz--;
	}

#if !defined(__aarch64__)
	memset((char *)d, 0, MASK_TO_NATURAL_SIZE(sz, clrblk_t));
	d += MASK_TO_NATURAL_SIZE(sz, clrblk_t);
	sz -= MASK_TO_NATURAL_SIZE(sz, clrblk_t);

#endif
	while (sz) {
		*d = 0;
		d++;
		sz--;
	}
}

struct uchunk {
	int64_t a[2];
};

typedef struct uchunk tfrblk;
#define tfrblksz sizeof(tfrblk)

void sx_wrpkmem(void *dst, const void *src, size_t sz)
{
	volatile char *d = (volatile char *)dst;
	volatile const char *s = (volatile const char *)src;

	while (sz && (!IS_NATURAL_ALIGNED(d, tfrblk) || !IS_NATURAL_SIZE(sz, tfrblk))) {
		*d = *s;
		d++;
		s++;
		sz--;
	}
	memcpy((char *)d, (char *)s, sz);
}

void sx_rdpkmem(void *dst, const void *src, size_t sz)
{
	if (IS_SAME_ALIGNMENT(dst, src, tfrblk) && IS_NATURAL_SIZE(sz, tfrblk)) {
		memcpy(dst, src, sz);
	} else {
		volatile char *d = (volatile char *)dst;
		volatile const char *s = (volatile const char *)src;

		while (sz) {
			*d = *s;
			d++;
			s++;
			sz--;
		}
	}
}
