/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*!
 * \brief compare memory areas in constant time.
 *
 * \param[in] s1 Pointer first memory area.
 * \param[in] s2 Pointer second memory area.
 * \param[in] n  Bytes that should be compared.
 *
 * \retval  Zero if the first n bytes of s1 and s2 are equal.
 */
int constant_memcmp(const void *s1, const void *s2, size_t n);

/*!
 * \brief Compare memory area to zero in constant time.
 *
 * \param[in] s1 Pointer to memory area.
 * \param[in] n  Bytes that should be compared.
 *
 * \return  True if all n bytes of s1 are zero, false otherwise.
 */
bool constant_memcmp_is_zero(const void *s1, size_t n);

/*!
 * \brief Compare array to a constant value in constant time.
 *
 * This function checks whether all elements of the array a are equal to the given value val,
 * using constant time.
 *
 * \param[in] a   Pointer to the array.
 * \param[in] val The value to compare against.
 * \param[in] sz  Number of elements in the array.
 *
 * \return 0 if all elements in a are equal to val, non-zero otherwise.
 */
int constant_memdiff_array_value(const uint8_t *a, uint8_t val, size_t sz);

/**
 * @brief Copy binary buffer based on selection.
 *
 * This function operates in constant time.
 *
 * @param[in]  select	 Value that identifies which buffer to copy.
 * @param[in]  true_val  Buffer to copy to @p dst if @p select is true.
 * @param[in]  false_val Buffer to copy to @p dst if @p select is false.
 * @param[out] dst	 Destination buffer.
 * @param[in]  sz	 Number of bytes to copy.
 */
void constant_select_bin(bool select, const uint8_t *true_val, const uint8_t *false_val,
			 uint8_t *dst, size_t sz);

/*!
 * \brief A memory set that is not optimized out by the compiler.
 *
 * \param[in] dest		Pointer to the memory to set.
 * \param[in] dest_size Size of the memory to set in bytes.
 * \param[in] ch		Fill byte.
 * \param[in] n		Bytes to be filled.
 *
 */
void safe_memset(void *dest, const size_t dest_size, const uint8_t ch, const size_t n);

/*!
 * \brief A memset to zero function that is not optimized out by the compiler.
 *
 * \param[in] dest		Pointer to the memory to set to zero.
 * \param[in] dest_size Size of the memory in bytes.
 *
 */
void safe_memzero(void *dest, const size_t dest_size);

/*!
 * \brief Will copy memory, verifying that the source buffer is non-zero.
 *
 * \param[in] dest      Pointer to destination memory.
 * \param[in] dest_size Size of the memory in bytes.
 * \param[in] src       Pointer to source memory.
 * \param[in] src_size  Size of the memory in bytes.
 *
 * \return true if src buffer is non-zero and fits into the destination.
 */
bool memcpy_check_non_zero(void *dest, size_t dest_size, const void *src, size_t src_size);
