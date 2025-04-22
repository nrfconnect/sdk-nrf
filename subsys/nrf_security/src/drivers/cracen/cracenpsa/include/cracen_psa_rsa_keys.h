/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

 #ifndef CRACEN_PSA_RSA_KEYS_H
 #define CRACEN_PSA_RSA_KEYS_H

#include <silexpk/core.h>
#include <silexpk/cmddefs/modexp.h>
#include <silexpk/cmddefs/rsa.h>
#include <silexpk/sxbuf/sxbufop.h>

/** Initialize an RSA key consisting of modulus and exponent. */
#define CRACEN_KEY_INIT_RSA(mod, expon)                                                                \
	(struct cracen_rsa_key)                                                                        \
	{                                                                                          \
		.cmd = SX_PK_CMD_MOD_EXP, .slotmask = (1 | (1 << 2)), .dataidx = 1, {mod, expon},  \
	}

/** Initialize an RSA key consisting of modulus and exponent. */
#define CRACEN_KEY_INIT_RSA(mod, expon)                                                                \
(struct cracen_rsa_key)                                                                        \
{                                                                                          \
	.cmd = SX_PK_CMD_MOD_EXP, .slotmask = (1 | (1 << 2)), .dataidx = 1, {mod, expon},  \
}


/** Initialize an RSA CRT key consisting of 2 primes and derived values.
 *
 * The 2 primes (p and q) must comply with the rules laid out in
 * the latest version of FIPS 186. This includes that the 2 primes must
 * have the highest bit set in their most significant byte. The full
 * key size in bits must be a multiple of 8.
 */
#define CRACEN_KEY_INIT_RSACRT(p, q, dp, dq, qinv)                                                     \
	(struct cracen_rsa_key)                                                                        \
	{                                                                                          \
		.cmd = SX_PK_CMD_MOD_EXP_CRT, .slotmask = (0x3 | (0x7 << 3)), .dataidx = 2,        \
		{p, q, dp, dq, qinv},                                                              \
	}

#endif /* CRACEN_PSA_RSA_KEYS_H */