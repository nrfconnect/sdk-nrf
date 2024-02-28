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
 * \brief A meset to zero function that is not optimized out by the compiler.
 *
 * \param[in] dest		Pointer to the memory to set to zero.
 * \param[in] dest_size Size of the memory in bytes.
 *
 */
void safe_memzero(void *dest, const size_t dest_size);
