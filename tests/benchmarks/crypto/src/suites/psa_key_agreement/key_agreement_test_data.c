/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "key_agreement_test_logic.h"

const struct suite suite_ecdh_brainpoolp256r1 =
	KEY_AGREEMENT_SUITE_ENTRY("ecdh", "brainpoolp256r1", PSA_ECC_FAMILY_BRAINPOOL_P_R1,
				  256, 65, 32);
const struct suite suite_ecdh_brainpoolp384r1 =
	KEY_AGREEMENT_SUITE_ENTRY("ecdh", "brainpoolp384r1", PSA_ECC_FAMILY_BRAINPOOL_P_R1,
				  384, 97, 48);
const struct suite suite_ecdh_brainpoolp512r1 =
	KEY_AGREEMENT_SUITE_ENTRY("ecdh", "brainpoolp512r1", PSA_ECC_FAMILY_BRAINPOOL_P_R1,
				  512, 129, 64);
const struct suite suite_ecdh_secp224r1 =
	KEY_AGREEMENT_SUITE_ENTRY("ecdh", "secp224r1", PSA_ECC_FAMILY_SECP_R1, 224, 57, 28);
const struct suite suite_ecdh =
	KEY_AGREEMENT_SUITE_ENTRY("ecdh", "secp256r1", PSA_ECC_FAMILY_SECP_R1, 256, 65, 32);
const struct suite suite_ecdh_secp384r1 =
	KEY_AGREEMENT_SUITE_ENTRY("ecdh", "secp384r1", PSA_ECC_FAMILY_SECP_R1, 384, 97, 48);
const struct suite suite_ecdh_secp521r1 =
	KEY_AGREEMENT_SUITE_ENTRY("ecdh", "secp521r1", PSA_ECC_FAMILY_SECP_R1, 521, 133, 66);
const struct suite suite_ecdh_x25519 =
	KEY_AGREEMENT_SUITE_ENTRY("ecdh", "x25519", PSA_ECC_FAMILY_MONTGOMERY, 255, 32, 32);
const struct suite suite_ecdh_x448 =
	KEY_AGREEMENT_SUITE_ENTRY("ecdh", "x448", PSA_ECC_FAMILY_MONTGOMERY, 448, 56, 56);
const struct suite suite_ecdh_secp256k1 =
	KEY_AGREEMENT_SUITE_ENTRY("ecdh", "secp256k1", PSA_ECC_FAMILY_SECP_K1, 256, 65, 32);
