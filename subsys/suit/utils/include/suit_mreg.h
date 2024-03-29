/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_MREG_H__
#define SUIT_MREG_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief A generic memory region representation.
 *
 * This type is similar to the zcbor string structure and was introduced in order to decouple
 * SUIT module interfaces from ZCBOR library.
 */
typedef struct {
	/** @brief Pointer to the memory region. */
	const uint8_t *mem;
	/** @brief Size of the memory region. */
	size_t size;
} suit_plat_mreg_t;

#ifdef __cplusplus
}
#endif

#endif /* SUIT_MREG_H__ */
