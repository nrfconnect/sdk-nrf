/*
 *  Copyright (c) 2025 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <silexpk/core.h>
#include <silexpk/montgomery.h>
#include <cracen_psa_montgomery.h>

/* U-coordinate of the base point, from RFC 7748. */
static const struct sx_x448_op x448_base = {.bytes = {5}};

/* U-coordinate of the base point, from RFC 7748. */
static const struct sx_x25519_op x25519_base = {.bytes = {9}};

int cracen_x25519_genpubkey(const uint8_t *priv_key, uint8_t *pub_key)
{
	return sx_x25519_ptmult((const struct sx_x25519_op *)priv_key, &x25519_base,
				(struct sx_x25519_op *)pub_key);
}

int cracen_x448_genpubkey(const uint8_t *priv_key, uint8_t *pub_key)
{
	return sx_x448_ptmult((const struct sx_x448_op *)priv_key, &x448_base,
			      (struct sx_x448_op *)pub_key);
}
