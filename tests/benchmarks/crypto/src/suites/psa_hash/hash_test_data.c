/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "hash_test_logic.h"

const struct suite suite_sha1 = HASH_SUITE_ENTRY("sha1", PSA_ALG_SHA_1);
const struct suite suite_sha224 = HASH_SUITE_ENTRY("sha224", PSA_ALG_SHA_224);
const struct suite suite_sha256 = HASH_SUITE_ENTRY("sha256", PSA_ALG_SHA_256);
const struct suite suite_sha384 = HASH_SUITE_ENTRY("sha384", PSA_ALG_SHA_384);
const struct suite suite_sha512 = HASH_SUITE_ENTRY("sha512", PSA_ALG_SHA_512);
const struct suite suite_sha3_224 = HASH_SUITE_ENTRY("sha3_224", PSA_ALG_SHA3_224);
const struct suite suite_sha3_256 = HASH_SUITE_ENTRY("sha3_256", PSA_ALG_SHA3_256);
const struct suite suite_sha3_384 = HASH_SUITE_ENTRY("sha3_384", PSA_ALG_SHA3_384);
const struct suite suite_sha3_512 = HASH_SUITE_ENTRY("sha3_512", PSA_ALG_SHA3_512);
const struct suite suite_shake256_512 =
	HASH_SUITE_ENTRY("shake256_512", PSA_ALG_SHAKE256_512);
