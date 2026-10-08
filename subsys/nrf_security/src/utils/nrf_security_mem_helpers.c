/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_security_mem_helpers.h>
#include <zephyr/sys/util.h>

#if defined(__GNUC__) || defined(__clang__)
#define SAFE_MEMZERO_WORDWISE
/* uint32_t that may alias any object type, like uint8_t. */
typedef uint32_t __attribute__((__may_alias__)) safe_memzero_word_t;
#define SAFE_MEMZERO_WORD_SIZE	sizeof(safe_memzero_word_t)
/* Four words are cleared per pass of the unrolled loop in safe_memzero(). */
#define SAFE_MEMZERO_BLOCK_SIZE (4U * SAFE_MEMZERO_WORD_SIZE)
#endif /* defined(__GNUC__) || defined(__clang__) */

int constant_memcmp(const void *s1, const void *s2, size_t n)
{
	const volatile uint8_t *a = s1;
	const volatile uint8_t *b = s2;
	volatile uint8_t x = 0;

	for (size_t i = 0; i < n; i++) {
		x |= a[i] ^ b[i];
	}

	return x;
}

bool constant_memcmp_is_zero(const void *s1, size_t n)
{
	const volatile uint8_t *a = s1;
	volatile uint8_t x = 0;

	for (size_t i = 0; i < n; i++) {
		x |= a[i];
	}

	return (x == 0);
}

int constant_memdiff_array_value(const uint8_t *a, uint8_t val, size_t sz)
{
	size_t i;
	int r = 0;

	for (i = 0; i < sz; i++) {
		r |= a[i] ^ val;
	}

	return r;
}

size_t constant_count(const uint8_t *a, size_t sz, uint8_t val, enum constant_count_match match)
{
	volatile size_t count = 0;
	volatile int still_matching = 1;

	for (size_t i = 0; i < sz; i++) {
		int matches = (match == MATCH_EQUAL) ? (a[i] == val) : (a[i] != val);
		int increment = still_matching & matches;
		size_t mask = (size_t)0 - (size_t)increment;

		count = (count & ~mask) | ((count + 1) & mask);
		still_matching &= matches;
	}

	return count;
}

void constant_mask_select_bin(uint8_t mask, const uint8_t *true_val, const uint8_t *false_val,
			      uint8_t *dst, size_t sz)
{
	for (size_t i = 0; i < sz; i++) {
		dst[i] = (mask & true_val[i]) | (~mask & false_val[i]);
	}
}

void constant_select_bin(bool select, const uint8_t *true_val, const uint8_t *false_val,
			 uint8_t *dst, size_t sz)
{
	uint8_t mask = select ? 0xFF : 0x00;

	constant_mask_select_bin(mask, true_val, false_val, dst, sz);
}

bool memcpy_check_non_zero(void *dest, size_t dest_size, const void *src, size_t src_size)
{
	uint8_t *byte_ptr_dest = dest;
	const uint8_t *byte_ptr_src = src;
	uint8_t x = 0;

	if (src_size > dest_size) {
		return false;
	}

	for (size_t i = 0; i < src_size; i++) {
		uint8_t byte = byte_ptr_src[i];

		byte_ptr_dest[i] = byte;
		x |= byte;
	}

	return (x != 0);
}

void safe_memset(void *dest, const size_t dest_size, const uint8_t ch, const size_t n)
{
	size_t bytes_to_set = MIN(dest_size, n);
	/* convert to volatile pointer to prevent to be optimized out*/
	volatile uint8_t *byte_ptr = dest;

	while (bytes_to_set--) {
		*byte_ptr = ch;
		byte_ptr++;
	}
}

void safe_memzero(void *dest, const size_t dest_size)
{
#ifdef SAFE_MEMZERO_WORDWISE
	/** Volatile access to every store restricts the compiler from removing
	 *  them and from turning the loops into a byte-wise memset() calls.
	 */
	volatile uint8_t *p = dest;
	size_t n = dest_size;

	/* Clearing single bytes up to the first word boundary. */
	for (; (n > 0U) && (((uintptr_t)p % SAFE_MEMZERO_WORD_SIZE) != 0U); n--, p++) {
		*p = 0U;
	}

	/* Clearing four aligned words per pass. */
	for (; n >= SAFE_MEMZERO_BLOCK_SIZE;
	     n -= SAFE_MEMZERO_BLOCK_SIZE, p += SAFE_MEMZERO_BLOCK_SIZE) {
		volatile safe_memzero_word_t *w = (volatile safe_memzero_word_t *)p;

		w[0] = 0U;
		w[1] = 0U;
		w[2] = 0U;
		w[3] = 0U;
	}

	/* Clearing remaining whole words (at most three). */
	for (; n >= SAFE_MEMZERO_WORD_SIZE;
	     n -= SAFE_MEMZERO_WORD_SIZE, p += SAFE_MEMZERO_WORD_SIZE) {
		*(volatile safe_memzero_word_t *)p = 0U;
	}

	/* Clearing trailing bytes (at most three). */
	for (; n > 0U; n--, p++) {
		*p = 0U;
	}
#else
	volatile uint8_t *volatile byte_pnt = (volatile uint8_t *volatile)dest;
	size_t i = (size_t)0U;

	for (; i < dest_size; i++) {
		byte_pnt[i] = 0U;
	}
#endif /* SAFE_MEMZERO_WORDWISE */
}
