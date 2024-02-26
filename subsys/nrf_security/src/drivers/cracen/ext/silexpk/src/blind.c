/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <silexpk/core.h>
#include <silexpk/blinding.h>
#include "internal.h"

void sx_pk_cnx_configure_blinding(struct sx_pk_cnx *cnx, struct sx_pk_blinder *prng)
{
	const struct sx_pk_capabilities *caps = sx_pk_fetch_capabilities();

	if (!prng || !caps->blinding) {
		return;
	}
	struct sx_pk_blinder **p = sx_pk_get_blinder(cnx);
	*p = prng;
}

sx_pk_blind_factor sx_pk_default_blind_gen(struct sx_pk_blinder *blinder)
{
	/* source: https://en.wikipedia.org/wiki/Xorshift xorshift64() */
	sx_pk_blind_factor x = blinder->state.blind;

	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return blinder->state.blind = x;
}

void sx_pk_default_blinder(struct sx_pk_blinder *blinder, sx_pk_blind_factor seed)
{
	blinder->state.blind = seed;
	blinder->generate = &sx_pk_default_blind_gen;
}
