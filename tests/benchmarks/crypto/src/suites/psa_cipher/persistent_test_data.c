/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "cipher_test_logic.h"

const struct suite suite_persistent = {.alg = "persistent", .keydesc = "aes128",
	.context = &(const struct cipher_test_data){.algorithm = PSA_ALG_CTR,
		.persistent_id = PSA_KEY_ID_USER_MIN, .key_bits = 128, .has_singlepart = true},
	.keysetup = {cipher_keysetup_ops, CIPHER_KEYSETUP_OP_COUNT},
	.singlepart = {cipher_singlepart_ops, CIPHER_SINGLEPART_OP_COUNT}, .check = cipher_check,
	.cleanup = cipher_cleanup};
