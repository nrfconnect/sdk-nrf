/*
 *  Copyright (c) 2025 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/ecc/cracen_montgomery.h>

#include <silexpk/core.h>
#include <silexpk/montgomery.h>

/* U-coordinate of the base point, from RFC 7748. */
static const struct sx_x448_op x448_base = {.bytes = {5}};

/* U-coordinate of the base point, from RFC 7748. */
static const struct sx_x25519_op x25519_base = {.bytes = {9}};

int cracen_x25519_genpubkey(const uint8_t *priv_key, uint8_t *pub_key)
{
	int sx_status;
	sx_pk_req req;

	sx_pk_acquire_hw(&req);
	sx_status = sx_x25519_ptmult(&req, (const struct sx_x25519_op *)priv_key,
				  &x25519_base, (struct sx_x25519_op *)pub_key);
	sx_pk_release_req(&req);
	return sx_status;
}

int cracen_x448_genpubkey(const uint8_t *priv_key, uint8_t *pub_key)
{
	int sx_status;
	sx_pk_req req;

	sx_pk_acquire_hw(&req);
	sx_status = sx_x448_ptmult(&req, (const struct sx_x448_op *)priv_key,
				  &x448_base, (struct sx_x448_op *)pub_key);
	sx_pk_release_req(&req);
	return sx_status;
}
