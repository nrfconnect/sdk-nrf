/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "pake_test_logic.h"

/* The combination samples/crypto/ecjpake demonstrates. */
const struct suite suite_ecjpake =
	ECJPAKE_SUITE_ENTRY("p256", PSA_ALG_SHA_256, PSA_ECC_FAMILY_SECP_R1, 256);

/* The combination samples/crypto/spake2p demonstrates. */
const struct suite suite_spake2p =
	SPAKE2P_SUITE_ENTRY("p256", PSA_ALG_SHA_256, PSA_ECC_FAMILY_SECP_R1, 256);

/* The only combination Cracen's SRP-6 accepts, so there is no second entry. */
const struct suite suite_srp6 =
	SRP6_SUITE_ENTRY("rfc3526_3072", PSA_ALG_SHA_512, PSA_DH_FAMILY_RFC3526, 3072);
