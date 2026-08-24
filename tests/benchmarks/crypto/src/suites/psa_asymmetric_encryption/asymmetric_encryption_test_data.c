/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "asymmetric_encryption_test_logic.h"

const struct suite suite_rsa_oaep = ASYMMETRIC_ENCRYPTION_SUITE_ENTRY(
	"rsa_oaep", PSA_ALG_RSA_OAEP(PSA_ALG_SHA_256));
const struct suite suite_rsa_pkcs1v15_crypt = ASYMMETRIC_ENCRYPTION_SUITE_ENTRY(
	"rsa_pkcs1v15_crypt", PSA_ALG_RSA_PKCS1V15_CRYPT);