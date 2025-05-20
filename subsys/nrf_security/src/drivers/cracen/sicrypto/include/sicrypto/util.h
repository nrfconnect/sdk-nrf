/*
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_UTIL_HEADER_FILE
#define SICRYPTO_UTIL_HEADER_FILE

#include <stddef.h>

/* Return 1, 0 or -1 if (a + carry) is greater than b, equal to b or less than
 * b, respectively.
 *
 * Inputs a and b are unsigned integers stored as big endian byte arrays of sz
 * bytes. The absolute value of the carry argument must be strictly smaller than
 * 0x100.
 */
int si_be_cmp(const unsigned char *a, const unsigned char *b, size_t sz, int carry);

/* Compute v = v + summand.
 *
 * v is an unsigned integer stored as a big endian byte array of sz bytes.
 * Summand must be less than or equal to the maximum value of a size_t minus 255.
 * The final carry is discarded: addition is modulo 2^(sz*8).
 */
void si_be_add(unsigned char *v, size_t sz, size_t summand);

void si_xorbytes(char *a, const char *b, size_t sz);
#endif
