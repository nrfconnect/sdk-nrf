/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <silexpk/iomem.h>

static void write_incomplete_word(uint32_t *dst, const uint8_t *bytes,
				  size_t first_byte_pos, size_t num_bytes)
{
	uint32_t word = *dst;

	for (size_t i = 0; i != num_bytes; ++i) {
		((uint8_t *)&word)[first_byte_pos + i] = bytes[i];
	}

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("write_incomplete_word(%p, 0x%x, %zu, %zu): 0x%x to 0x%x\r\n",
		dst, *(uint32_t *)bytes, first_byte_pos, num_bytes, *dst, word);
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
		*(uint8_t **)&dst += byte_count;
		*(uint8_t **)&src += byte_count;
		sz -= byte_count;
	}

	for (size_t i = 0; i != sz / 4; ++i) {
		((uint32_t *)dst)[i] = ((uint32_t *)src)[i];
	}

	if (sz % 4) {
		write_incomplete_word((uint32_t *)((uint8_t *)dst + (sz & ~3)),
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
