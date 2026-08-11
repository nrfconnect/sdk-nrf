/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Bit-stream packing primitives shared by the CRACEN lattice-based
 *        algorithms.
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * @details
 * Both FIPS 204 (BitsToBytes / BytesToBits, Section 7.1) and FIPS 203
 * (ByteEncode / ByteDecode, Section 4.2.1) encode arrays of small integers the
 * same way: bits are ordered least-significant-first within each byte and every
 * integer is stored little-endian.
 *
 * The reader and writer below stream the packed bytes through a 64-bit
 * accumulator, loading and storing one 32-bit word at a time. They rely on the
 * packed data being a whole number of 32-bit words long: the reader then never
 * loads past the end of the packed buffer, and the writer's accumulator is
 * exactly empty after the last integer. This holds for every coefficient width
 * used by ML-DSA and ML-KEM, since a 256-coefficient polynomial packed with
 * c bits per coefficient occupies 32 * c bytes.
 */

#ifndef CRACEN_PQC_BITS_H
#define CRACEN_PQC_BITS_H

#include <stddef.h>
#include <stdint.h>
#include <cracen/common.h>

/** @brief Reader state of a packed bit stream. Initialize @c next to the packed
 *         buffer and the remaining fields to 0.
 */
struct pqc_bit_reader {
	const uint8_t *next; /* next word to load */
	uint64_t acc;	     /* not yet consumed bits, LSB first */
	uint32_t acc_bits;   /* number of valid bits in acc */
};

/** @brief Writer state of a packed bit stream. Initialize @c next to the output
 *         buffer and the remaining fields to 0.
 */
struct pqc_bit_writer {
	uint8_t *next;	   /* next word to store */
	uint64_t acc;	   /* not yet stored bits, LSB first */
	uint32_t acc_bits; /* number of valid bits in acc */
};

/** @brief Read specified number of bits as a little-endian unsigned integer.
 *
 * @param[in,out] r  Reader state.
 * @param[in] n      Number of bits to read, at most 32.
 *
 * @return The integer that was read.
 */
static inline uint32_t cracen_pqc_read_bits(struct pqc_bit_reader *r, uint32_t n)
{
	uint32_t val;

	if (r->acc_bits < n) {
		r->acc |= (uint64_t)cracen_get_le32(r->next) << r->acc_bits;
		r->next += sizeof(uint32_t);
		r->acc_bits += 32;
	}

	val = (uint32_t)(r->acc & (((uint64_t)1 << n) - 1u));
	r->acc >>= n;
	r->acc_bits -= n;

	return val;
}

/** @brief Write the low bits of the specified value, little-endian.
 *
 * @param[in,out] w  Writer state.
 * @param[in] val    Value to write.
 * @param[in] n      Number of bits to write, at most 32.
 */
static inline void cracen_pqc_write_bits(struct pqc_bit_writer *w, uint32_t val, uint32_t n)
{
	w->acc |= ((uint64_t)val & (((uint64_t)1 << n) - 1u)) << w->acc_bits;
	w->acc_bits += n;

	if (w->acc_bits >= 32) {
		cracen_put_le32((uint32_t)w->acc, w->next);
		w->next += sizeof(uint32_t);
		w->acc >>= 32;
		w->acc_bits -= 32;
	}
}

/**
 * @brief Computes the bit length of a positive integer x
 *	  (see bitlen - FIPS 204, Section 2.3).
 *
 * @param[in] x Positive integer.
 *
 * @return The number of digits that would appear in a base-2 representation of x,
 *	   where the most significant digit in the representation is assumed to be a 1.
 */
static inline uint32_t cracen_pqc_bit_length(uint32_t x)
{
	if (x == 0) {
		return 0;
	}

	return 32u - __builtin_clz(x);
}

#endif /* CRACEN_PQC_BITS_H */
