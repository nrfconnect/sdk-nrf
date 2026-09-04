/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "key_derivation_test_logic.h"

/* The shared salt and info live in key_derivation_test_logic.c. */

const struct suite suite_hkdf = KEY_DERIVATION_SUITE_ENTRY(
	"hkdf", "sha256", PSA_ALG_HKDF(PSA_ALG_SHA_256), PSA_KEY_TYPE_DERIVE, 256, 0, 42);

/* The only password-based one, so the only one with a cost. Kept at one for
 * speed; a real registration uses a high iteration count.
 */
const struct suite suite_pbkdf2 = KEY_DERIVATION_SUITE_ENTRY(
	"pbkdf2", "sha256", PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256), PSA_KEY_TYPE_PASSWORD, 128, 1,
	64);

/* 48 bytes: the TLS 1.2 master secret length. */
const struct suite suite_tls12_prf = KEY_DERIVATION_SUITE_ENTRY(
	"tls12_prf", "sha256", PSA_ALG_TLS12_PRF(PSA_ALG_SHA_256), PSA_KEY_TYPE_DERIVE, 256, 0, 48);

const struct suite suite_tls12_psk_to_ms = KEY_DERIVATION_SUITE_ENTRY(
	"tls12_psk_to_ms", "sha256", PSA_ALG_TLS12_PSK_TO_MS(PSA_ALG_SHA_256),
	PSA_KEY_TYPE_DERIVE, 256, 0, 48);
