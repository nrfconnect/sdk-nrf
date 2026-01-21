/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <silexpk/iomem.h>

#ifndef CONFIG_SOC_NRF54LM20A

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
	volatile uint8_t *dst_ptr = (volatile uint8_t *)dst;

	if (sz == 0) {
		return;
	}

	while (sz && (!IS_NATURAL_ALIGNED(dst_ptr, clrblk_t))) {
		*dst_ptr = 0;
		dst_ptr++;
		sz--;
	}

#if !defined(__aarch64__)
	memset((uint8_t *)dst_ptr, 0, MASK_TO_NATURAL_SIZE(sz, clrblk_t));
	dst_ptr += MASK_TO_NATURAL_SIZE(sz, clrblk_t);
	sz -= MASK_TO_NATURAL_SIZE(sz, clrblk_t);

#endif
	while (sz) {
		*dst_ptr = 0;
		dst_ptr++;
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
	volatile uint8_t *dst_ptr = (volatile uint8_t *)dst;
	volatile const uint8_t *src_ptr = (volatile const uint8_t *)src;

	while (sz && (!IS_NATURAL_ALIGNED(dst_ptr, tfrblk) || !IS_NATURAL_SIZE(sz, tfrblk))) {
		*dst_ptr = *src_ptr;
		dst_ptr++;
		src_ptr++;
		sz--;
	}
	memcpy((uint8_t *)dst_ptr, (uint8_t *)src_ptr, sz);
}

#else /* 54LM20A requires word-aligned, word-sized memory accesses */

static void write_incomplete_word(uint32_t *dst, const uint8_t *bytes,
				  size_t first_byte_pos, size_t num_bytes)
{
	uint32_t word = *dst;

	for (size_t i = 0; i != num_bytes; ++i) {
		((uint8_t *)&word)[first_byte_pos + i] = bytes[i];
	}

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("write_incomplete_word(%p, 0x%x, %zu, %zu): 0x%x to 0x%x\r\n",
		dst, *(const uint32_t *)bytes, first_byte_pos, num_bytes, *dst, word);
#endif
	if ((uintptr_t)dst % 4) {
		SX_WARN_UNALIGNED_ADDR(dst);
	}

	*dst = word;
}

void sx_clrpkmem(void *dst, size_t sz)
{
	if (sz == 0) {
		return;
	}
#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_clrpkmem(%p, %zu)\r\n", dst, sz);
#endif

	const uint8_t zero[4] = {};
	const uintptr_t dst_addr = (uintptr_t)dst;

	if (dst_addr % 4) {
		const size_t first_byte_pos = dst_addr % 4;
		const size_t byte_count = 4 - first_byte_pos;

		write_incomplete_word((uint32_t *)(dst_addr & ~3), zero,
				      first_byte_pos, byte_count);
		dst = (void *)(dst_addr + byte_count);
		sz -= byte_count;
	}
	uint32_t *word_dst = (uint32_t *)dst;

	for (size_t i = 0; i != sz / 4; ++i) {
		word_dst[i] = 0;
	}
	if (sz % 4) {
		write_incomplete_word(&word_dst[sz / 4], zero, 0, sz % 4);
	}
}

void sx_wrpkmem(void *dst, const void *src, size_t sz)
{
#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_wrpkmem(%p, %p, %zu)\r\n", dst, src, sz);
#endif

	if ((uintptr_t)dst % 4) {
		const uintptr_t dst_addr = (uintptr_t)dst;
		const size_t first_byte_pos = dst_addr % 4;
		const size_t byte_count = 4 - first_byte_pos;

		write_incomplete_word((uint32_t *)(dst_addr & ~3), src, first_byte_pos, byte_count);
		dst = (uint8_t *)dst + byte_count;
		src = (const uint8_t *)src + byte_count;
		sz -= byte_count;
	}

	/* Use memcpy to read from src since it may be unaligned.
	 * dst is guaranteed to be 4-byte aligned at this point.
	 */
	for (size_t i = 0; i != sz / 4; ++i) {
		uint32_t word;

		memcpy(&word, (const uint8_t *)src + i * 4, sizeof(word));
		((uint32_t *)dst)[i] = word;
	}

	if (sz % 4) {
		write_incomplete_word((uint32_t *)((uint8_t *)dst + (sz & ~3)),
				      (const uint8_t *)src + (sz & ~3), 0, sz % 4);
	}
}

void sx_wrpkmem_byte(void *dst, uint8_t input_byte)
{
	uintptr_t dst_addr = (uintptr_t)dst;
	uint32_t *word_dst = (uint32_t *)(dst_addr & ~3);
	uint32_t word = *word_dst;
	size_t byte_index = dst_addr % 4;

	((uint8_t *)&word)[byte_index] = input_byte;
	*word_dst = word;
}

#endif
