/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "aead_test_logic.h"

const struct suite suite_aes_ccm_128 = AEAD_SUITE_ENTRY(
	"aes_ccm", "aes128", PSA_ALG_CCM, PSA_KEY_TYPE_AES, 128, 13);
const struct suite suite_aes_ccm_192 = AEAD_SUITE_ENTRY(
	"aes_ccm", "aes192", PSA_ALG_CCM, PSA_KEY_TYPE_AES, 192, 13);
const struct suite suite_aes_ccm_256 = AEAD_SUITE_ENTRY(
	"aes_ccm", "aes256", PSA_ALG_CCM, PSA_KEY_TYPE_AES, 256, 13);

const struct suite suite_aes_gcm_128 = AEAD_SUITE_ENTRY(
	"aes_gcm", "aes128", PSA_ALG_GCM, PSA_KEY_TYPE_AES, 128, 12);
const struct suite suite_aes_gcm_192 = AEAD_SUITE_ENTRY(
	"aes_gcm", "aes192", PSA_ALG_GCM, PSA_KEY_TYPE_AES, 192, 12);
const struct suite suite_aes_gcm_256 = AEAD_SUITE_ENTRY(
	"aes_gcm", "aes256", PSA_ALG_GCM, PSA_KEY_TYPE_AES, 256, 12);

const struct suite suite_chachapoly = AEAD_SUITE_ENTRY(
	"chachapoly", "chacha20", PSA_ALG_CHACHA20_POLY1305,
	PSA_KEY_TYPE_CHACHA20, 256, 12);
