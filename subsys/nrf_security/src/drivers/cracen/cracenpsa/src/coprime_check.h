/*
 *  Copyright (c) 2025 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef COPRIME_CHECK_HEADER_FILE
#define COPRIME_CHECK_HEADER_FILE

#include <stdint.h>

/** Function to check if two integer numbers are coprime.
 * @param[in] a            Unsigned integer stored as a big endian byte array.
 * @param[in] asz          Size in bytes of \p a.
 * @param[in] b            Unsigned integer stored as a big endian byte array.
 * @param[in] bsz          Size in bytes of \p b.
 *
 * At least one of the two input numbers \p a, \p b must be odd.
 *
 * The most significant byte in \p a and in \p b must not be zero.
 *
 * On function completion, the value of the returned status code indicates whether the
 * given numbers \p a and \p b are coprime (the status code equals SX_OK) or not
 * (the status code equals SX_ERR_NOT_INVERTIBLE).
 *
 * This function needs a workmem buffer with size:
 *      min(asz, bsz)
 * where all sizes are expressed in bytes.
 */

int cracen_coprime_check(uint8_t *workmem, size_t workmemsz, const uint8_t *a, size_t asz,
			 const uint8_t *b, size_t bsz);

#endif
