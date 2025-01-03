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

static void write_incomplete_word(uint32_t *dst, const uint8_t *bytes,
				  size_t first_byte_pos, size_t num_bytes)
{
	if ((uintptr_t)dst % 4) {
		SX_WARN_UNALIGNED_ADDR(dst);
	}
	uint32_t word = *dst;

	for (size_t i = 0; i != num_bytes; ++i) {
		((uint8_t *)&word)[first_byte_pos + i] = bytes[i];
	}

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("write_incomplete_word(%p, 0x%x, %zu, %zu): 0x%x to 0x%x\r\n",
		dst, *(uint32_t *)bytes, first_byte_pos, num_bytes, *dst, word);
#endif

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
	if ((uintptr_t)dst % 4) {
		SX_WARN_UNALIGNED_ADDR(dst);
	}
	uint32_t *word_dst = (uint32_t *)dst;

	for (size_t i = 0; i != sz / 4; ++i) {
		word_dst[i] = 0;
	}
	if (sz % 4) {
		uint32_t zero = 0;

		write_incomplete_word(&word_dst[sz / 4], (uint8_t *)&zero, 0, sz % 4);
	}
}

void sx_wrpkmem(void *dst, const void *src, size_t sz)
{
#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_wrpkmem(%p, %zu)\r\n", dst, sz);
#endif
	uintptr_t dst_addr = (uintptr_t)dst;

	if (dst_addr % 4) {
		const size_t byte_count = 4 - dst_addr % 4;

		write_incomplete_word((uint32_t *)(dst_addr & ~3), src, dst_addr % 4, byte_count);
		*(uint8_t **)&dst += byte_count;
		*(uint8_t **)&src += byte_count;
		sz -= byte_count;
	}

	for (size_t i = 0; i != sz / 4; ++i) {
		((uint32_t *)dst)[i] = ((uint32_t *)src)[i];
	}

	if (sz % 4) {
		write_incomplete_word((uint32_t *)(dst_addr + (sz & ~3)),
				      (uint8_t *)src + (sz & ~3), 0, sz % 4);
	}
}

void sx_wrpkmem_byte(void *dst, char input_byte)
{
	uintptr_t dst_addr = (uintptr_t)dst;
	uint32_t *word_dst = (uint32_t *)(dst_addr & ~3);
	uint32_t word = *word_dst;
	size_t byte_index = dst_addr % 4;

	((uint8_t *)&word)[byte_index] = input_byte;
	*word_dst = word;
}
