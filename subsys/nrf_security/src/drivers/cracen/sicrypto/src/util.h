/*
 *  Copyright (c) 2020 Silex Insight
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UTIL_HEADER_FILE
#define UTIL_HEADER_FILE

#include <sicrypto/util.h>

/* Compute v = v + summand.
 *
 * v is an unsigned integer stored as a big endian byte array of sz bytes.
 * Summand must be less than or equal to the maximum value of a size_t minus 255.
 * The final carry is discarded: addition is modulo 2^(sz*8).
 */
void si_be_add(unsigned char *v, size_t sz, size_t summand);

void si_xorbytes(char *a, const char *b, size_t sz);

int si_status_on_prng(struct sitask *t);

int si_wait_on_prng(struct sitask *t);

int si_silexpk_status(struct sitask *t);

int si_silexpk_wait(struct sitask *t);

#endif
