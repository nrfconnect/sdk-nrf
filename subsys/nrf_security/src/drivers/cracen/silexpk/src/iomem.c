/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <silexpk/iomem.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

/* PK memory only allows naturally aligned accesses (54LM20A: aligned words only).
 * memcpy()/memset() don't guarantee their access sizes, so PK memory is accessed
 * as [head bytes][aligned words][tail bytes] with volatile accesses only.
 */

#define WORD_SIZE sizeof(uint32_t)

/* Byte position of addr within its word (0-3). */
static inline size_t offset_in_word(const void *addr)
{
	return (uintptr_t)addr % WORD_SIZE;
}

/* Address of the word that contains addr. */
static inline uintptr_t word_start(const void *addr)
{
	return (uintptr_t)addr - offset_in_word(addr);
}

/* Bytes before the first word-aligned address, at most sz. */
static inline size_t head_length(const void *pk, size_t sz)
{
	size_t offset = offset_in_word(pk);

	return MIN(offset ? WORD_SIZE - offset : 0, sz);
}

/* Write len bytes from src to pk. The bytes must be in one word. */
static inline void write_bytes(uint8_t *pk, const uint8_t *src, size_t len)
{
#if defined(CONFIG_PSA_NEED_CRACEN_MEMORY_ACCESS_WORKAROUND)
	/* One read-modify-write of the word (little-endian). */
	volatile uint32_t *pk_word = (volatile uint32_t *)word_start(pk);
	uint32_t shift = 8 * offset_in_word(pk);
	uint32_t word = *pk_word;

	for (size_t i = 0; i < len; i++, shift += 8) {
		word = (word & ~(0xFFu << shift)) | ((uint32_t)src[i] << shift);
	}
	*pk_word = word;
#else
	for (size_t i = 0; i < len; i++) {
		((volatile uint8_t *)pk)[i] = src[i];
	}
#endif
}

/* Read len bytes from pk into dst. The bytes must be in one word. */
static inline void read_bytes(uint8_t *dst, const uint8_t *pk, size_t len)
{
#if defined(CONFIG_PSA_NEED_CRACEN_MEMORY_ACCESS_WORKAROUND)
	/* One read of the word (little-endian). */
	uint32_t word = *(const volatile uint32_t *)word_start(pk);
	uint32_t shift = 8 * offset_in_word(pk);

	for (size_t i = 0; i < len; i++, shift += 8) {
		dst[i] = (uint8_t)(word >> shift);
	}
#else
	for (size_t i = 0; i < len; i++) {
		dst[i] = ((const volatile uint8_t *)pk)[i];
	}
#endif
}

void sx_clrpkmem(void *dst, size_t sz)
{
	static const uint8_t zeros[WORD_SIZE - 1];
	uint8_t *pk = dst;
	size_t head_len = head_length(pk, sz);
	size_t word_count = (sz - head_len) / WORD_SIZE;
	size_t tail_len = (sz - head_len) % WORD_SIZE;
	volatile uint32_t *pk_words;

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_clrpkmem(%p, %zu)\r\n", dst, sz);
#endif

	if (head_len > 0) {
		write_bytes(pk, zeros, head_len);
		pk += head_len;
	}

	pk_words = (volatile uint32_t *)pk;
	for (size_t i = 0; i < word_count; i++) {
		pk_words[i] = 0;
	}
	pk += word_count * WORD_SIZE;

	if (tail_len > 0) {
		write_bytes(pk, zeros, tail_len);
	}
}

void sx_wrpkmem(void *dst, const void *src, size_t sz)
{
	uint8_t *pk = dst;
	const uint8_t *ram = src;
	size_t head_len = head_length(pk, sz);
	size_t word_count = (sz - head_len) / WORD_SIZE;
	size_t tail_len = (sz - head_len) % WORD_SIZE;
	volatile uint32_t *pk_words;

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_wrpkmem(%p, %p, %zu)\r\n", dst, src, sz);
#endif

	if (head_len > 0) {
		write_bytes(pk, ram, head_len);
		pk += head_len;
		ram += head_len;
	}

	/* ram may be unaligned. */
	pk_words = (volatile uint32_t *)pk;
	for (size_t i = 0; i < word_count; i++) {
		pk_words[i] = UNALIGNED_GET((const uint32_t *)&ram[i * WORD_SIZE]);
	}
	pk += word_count * WORD_SIZE;
	ram += word_count * WORD_SIZE;

	if (tail_len > 0) {
		write_bytes(pk, ram, tail_len);
	}
}

void sx_rdpkmem(void *dst, const void *src, size_t sz)
{
	uint8_t *ram = dst;
	const uint8_t *pk = src;
	size_t head_len = head_length(pk, sz);
	size_t word_count = (sz - head_len) / WORD_SIZE;
	size_t tail_len = (sz - head_len) % WORD_SIZE;
	const volatile uint32_t *pk_words;

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_rdpkmem(%p, %p, %zu)\r\n", dst, src, sz);
#endif

	if (head_len > 0) {
		read_bytes(ram, pk, head_len);
		pk += head_len;
		ram += head_len;
	}

	/* ram may be unaligned. */
	pk_words = (const volatile uint32_t *)pk;
	for (size_t i = 0; i < word_count; i++) {
		UNALIGNED_PUT(pk_words[i], (uint32_t *)&ram[i * WORD_SIZE]);
	}
	pk += word_count * WORD_SIZE;
	ram += word_count * WORD_SIZE;

	if (tail_len > 0) {
		read_bytes(ram, pk, tail_len);
	}
}

void sx_wrpkmem_byte(void *dst, uint8_t input_byte)
{
	write_bytes(dst, &input_byte, 1);
}

uint8_t sx_rdpkmem_byte(const void *src)
{
	uint8_t value;

	read_bytes(&value, src, 1);
	return value;
}
