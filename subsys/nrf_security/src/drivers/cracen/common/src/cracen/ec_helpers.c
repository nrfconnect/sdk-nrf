/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>

void cracen_decode_u_coordinate_25519(uint8_t *u)
{
	u[31] &= ~0x80; /* Clear the last bit. */
}

void cracen_decode_scalar_25519(uint8_t *k)
{
	k[0] &= ~0x07; /* Clear bits 0, 1 and 2. */
	k[31] &= ~0x80; /* Clear bit 255. */
	k[31] |= 0x40; /* Set bit 254. */
}

void cracen_decode_scalar_448(uint8_t *k, size_t k_size)
{
	k[0] &= ~0x03; /* Clear bits 0 and 1. */
	if (k_size > 56) {
		k[56] = 0x00; /* Clear the last byte of an Ed448 scalar. */
	}
	k[55] |= 0x80; /* Set bit 447. */
}
