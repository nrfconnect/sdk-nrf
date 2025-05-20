/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>

void decode_u_coordinate_25519(uint8_t *u)
{
	u[31] &= ~0x80; /* Clear the last bit. */
}

void decode_scalar_25519(uint8_t *k)
{
	k[0] &= ~0x07; /* Clear bits 0, 1 and 2. */
	k[31] &= ~0x80; /* Clear bit 255. */
	k[31] |= 0x40; /* Set bit 254. */
}

void decode_scalar_448(uint8_t *k)
{
	k[0] &= ~0x03; /* Clear bits 0 and 1. */
	k[55] |= 0x80; /* Set bit 447. */
}
