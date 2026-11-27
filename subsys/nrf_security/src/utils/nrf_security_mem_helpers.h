/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef NRF_SECURITY_MEM_HELPERS_H__
#define NRF_SECURITY_MEM_HELPERS_H__

/** @file
 * @addtogroup nrf_security_mem_helpers
 * @{
 * @brief Generic constant-time memory utilities.
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Whether constant_count() should count elements equal or not equal to @p val.
 */
enum constant_count_match {
	MATCH_EQUAL,
	MATCH_NOT_EQUAL
};

/**
 * @brief Compare memory areas in constant time.
 *
 * @param[in] s1 Pointer to the first memory area.
 * @param[in] s2 Pointer to the second memory area.
 * @param[in] n  Number of bytes to compare.
 *
 * @return Zero if the first @p n bytes of @p s1 and @p s2 are equal.
 */
int constant_memcmp(const void *s1, const void *s2, size_t n);

/**
 * @brief Compare a memory area to zero in constant time.
 *
 * @param[in] s1 Pointer to the memory area.
 * @param[in] n  Number of bytes to compare.
 *
 * @return True if all @p n bytes of @p s1 are zero, false otherwise.
 */
bool constant_memcmp_is_zero(const void *s1, size_t n);

/**
 * @brief Compare an array to a constant value in constant time.
 *
 * This function checks whether all elements of the array @p a are equal to the
 * given value @p val, using constant time.
 *
 * @param[in] a   Pointer to the array.
 * @param[in] val The value to compare against.
 * @param[in] sz  Number of elements in the array.
 *
 * @return 0 if all elements in @p a are equal to @p val, non-zero otherwise.
 */
int constant_memdiff_array_value(const uint8_t *a, uint8_t val, size_t sz);

/**
 * @brief Count leading array elements matching (or not matching) a value, in constant time.
 *
 * Scans the whole array unconditionally, counting how many elements starting at index 0
 * satisfy the match condition before the first element that does not.
 *
 * @param[in] a     Pointer to the array.
 * @param[in] sz    Number of elements in the array.
 * @param[in] val   The value to compare each element against.
 * @param[in] match Whether to count elements equal or not equal to @p val.
 *
 * @return Number of leading matching elements. Equal to @p sz if all elements matched.
 */
size_t constant_count(const uint8_t *a, size_t sz, uint8_t val, enum constant_count_match match);

/**
 * @brief Copy binary buffer based on mask.
 *
 * This function operates in constant time.
 *
 * @param[in]  mask      Mask that identifies which buffer to copy (0xFF or 0x00).
 * @param[in]  true_val  Buffer to copy to @p dst if @p mask is 0xFF.
 * @param[in]  false_val Buffer to copy to @p dst if @p mask is 0x00.
 * @param[out] dst       Destination buffer.
 * @param[in]  sz        Number of bytes to copy.
 */
void constant_mask_select_bin(uint8_t mask, const uint8_t *true_val, const uint8_t *false_val,
			      uint8_t *dst, size_t sz);

/**
 * @brief Copy binary buffer based on selection.
 *
 * This function operates in constant time.
 *
 * @param[in]  select    Value that identifies which buffer to copy.
 * @param[in]  true_val  Buffer to copy to @p dst if @p select is true.
 * @param[in]  false_val Buffer to copy to @p dst if @p select is false.
 * @param[out] dst       Destination buffer.
 * @param[in]  sz        Number of bytes to copy.
 */
void constant_select_bin(bool select, const uint8_t *true_val, const uint8_t *false_val,
			 uint8_t *dst, size_t sz);

/**
 * @brief Set memory in a way that is not optimized out by the compiler.
 *
 * @param[in] dest      Pointer to the memory to set.
 * @param[in] dest_size Size of the memory to set in bytes.
 * @param[in] ch        Fill byte.
 * @param[in] n         Number of bytes to be filled.
 */
void safe_memset(void *dest, const size_t dest_size, const uint8_t ch, const size_t n);

/**
 * @brief Set memory to zero in a way that is not optimized out by the compiler.
 *
 * @param[in] dest      Pointer to the memory to set to zero.
 * @param[in] dest_size Size of the memory in bytes.
 */
void safe_memzero(void *dest, const size_t dest_size);

/**
 * @brief Copy @p src into @p dest while checking that the source is non-zero.
 *
 * Copies byte by byte, then returns whether every source byte was non-zero and
 * @p src_size did not exceed @p dest_size. On failure the destination may already
 * contain a partial copy.
 *
 * @param[in] dest      Pointer to destination memory.
 * @param[in] dest_size Size of the destination in bytes.
 * @param[in] src       Pointer to source memory.
 * @param[in] src_size  Size of the source in bytes.
 *
 * @return True if the source buffer is non-zero and fits into the destination.
 */
bool memcpy_check_non_zero(void *dest, size_t dest_size, const void *src, size_t src_size);

/** @} */

#endif /* NRF_SECURITY_MEM_HELPERS_H__ */
