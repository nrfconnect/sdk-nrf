/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/mem_helpers.h>
#include <zephyr/sys/util.h>

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

void constant_select_bin(bool select, const uint8_t *true_val, const uint8_t *false_val,
			 uint8_t *dst, size_t sz)
{
	int8_t mask = select ? 0xFF : 0x00;

	for (size_t i = 0; i < sz; i++) {
		dst[i] = (mask & true_val[i]) | (~mask & false_val[i]);
	}
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
	/* Solution is heavily based on the sodium_memzero function here:
	 * https://github.com/jedisct1/libsodium/blob/18fad78494956bef63db887f5a9efcc13b89e1a7/
	 * src/libsodium/sodium/utils.c#L125
	 * We use the fallback solution used on this project which seems to work at the moment.
	 * We need to note that this is not a perfect solution since compilers might still manage
	 * to ignore this memset so we might want to check that this is working if we decide to
	 * use another compiler than GCC.
	 */
	volatile uint8_t *volatile byte_pnt = (volatile uint8_t *volatile)dest;
	size_t i = (size_t)0U;

	for (; i < dest_size; i++) {
		byte_pnt[i] = 0U;
	}
}
