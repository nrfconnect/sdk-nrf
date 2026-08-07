/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <silexpk/iomem.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#if !defined(CONFIG_PSA_NEED_CRACEN_MEMORY_ACCESS_WORKAROUND)

#define PTR_ALIGNMENT(ptr, type)	(((uintptr_t)ptr) & (sizeof(type) - 1))
#define IS_NATURAL_ALIGNED(ptr, type)	!PTR_ALIGNMENT(ptr, type)

void sx_clrpkmem(void *dst, size_t sz)
{
	uint8_t *dst_ptr = (uint8_t *)dst;

	/* Align start address since PK memory is device memory */
	while (sz && !IS_NATURAL_ALIGNED(dst_ptr, uint32_t)) {
		*(volatile uint8_t *)dst_ptr = 0;
		dst_ptr++;
		sz--;
	}

	memset(dst_ptr, 0, sz);
}

void sx_wrpkmem(void *dst, const void *src, size_t sz)
{
	uint8_t *dst_ptr = (uint8_t *)dst;
	const uint8_t *src_ptr = (const uint8_t *)src;

	/* Align start address since PK memory is device memory */
	while (sz && !IS_NATURAL_ALIGNED(dst_ptr, uint32_t)) {
		*(volatile uint8_t *)dst_ptr = *src_ptr;
		dst_ptr++;
		src_ptr++;
		sz--;
	}
	memcpy(dst_ptr, src_ptr, sz);
}

void sx_rdpkmem(void *dst, const void *src, size_t sz)
{
	uint8_t *dst_ptr = (uint8_t *)dst;
	const uint8_t *src_ptr = (const uint8_t *)src;

	/* Align start address since PK memory is device memory */
	while (sz && !IS_NATURAL_ALIGNED(src_ptr, uint32_t)) {
		*dst_ptr = *(const volatile uint8_t *)src_ptr;
		dst_ptr++;
		src_ptr++;
		sz--;
	}

	memcpy(dst_ptr, src_ptr, sz);
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
		dst, UNALIGNED_GET((const uint32_t *)bytes), first_byte_pos, num_bytes, *dst, word);
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

	/* dst is guaranteed to be 4-byte aligned at this point.
	 * Use UNALIGNED_GET to read from src since it may be unaligned.
	 */
	for (size_t i = 0; i != sz / 4; ++i) {
		((uint32_t *)dst)[i] = UNALIGNED_GET((uint32_t *)((const uint8_t *)src + i * 4));
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

void sx_rdpkmem(void *dst, const void *src, size_t sz)
{
#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_rdpkmem(%p, %p, %zu)\r\n", dst, src, sz);
#endif

	uint8_t *d = dst;
	uintptr_t s = (uintptr_t)src;

	/* Always read aligned words from CRACEN, write bytes to dst */
	while (sz > 0) {
		uint32_t word = *(const uint32_t *)(s & ~3);
		size_t offset = s % 4;
		size_t count = MIN(4 - offset, sz);

		for (size_t i = 0; i < count; ++i) {
			d[i] = ((const uint8_t *)&word)[offset + i];
		}
		d += count;
		s += count;
		sz -= count;
	}
}

#endif
