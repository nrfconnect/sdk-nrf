/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "cipher_test_logic.h"

const struct suite suite_aes_cbc_128 = CIPHER_SUITE_ENTRY(
	"aes_cbc", "aes128", PSA_ALG_CBC_NO_PADDING, 128, true);
const struct suite suite_aes_cbc_192 = CIPHER_SUITE_ENTRY(
	"aes_cbc", "aes192", PSA_ALG_CBC_NO_PADDING, 192, true);
const struct suite suite_aes_cbc_256 = CIPHER_SUITE_ENTRY(
	"aes_cbc", "aes256", PSA_ALG_CBC_NO_PADDING, 256, true);

const struct suite suite_aes_cbc_pkcs7_128 = CIPHER_SUITE_ENTRY(
	"aes_cbc_pkcs7", "aes128", PSA_ALG_CBC_PKCS7, 128, true);
const struct suite suite_aes_cbc_pkcs7_192 = CIPHER_SUITE_ENTRY(
	"aes_cbc_pkcs7", "aes192", PSA_ALG_CBC_PKCS7, 192, true);
const struct suite suite_aes_cbc_pkcs7_256 = CIPHER_SUITE_ENTRY(
	"aes_cbc_pkcs7", "aes256", PSA_ALG_CBC_PKCS7, 256, true);

const struct suite suite_aes_ctr_128 = CIPHER_SUITE_ENTRY(
	"aes_ctr", "aes128", PSA_ALG_CTR, 128, true);
const struct suite suite_aes_ctr_192 = CIPHER_SUITE_ENTRY(
	"aes_ctr", "aes192", PSA_ALG_CTR, 192, true);
const struct suite suite_aes_ctr_256 = CIPHER_SUITE_ENTRY(
	"aes_ctr", "aes256", PSA_ALG_CTR, 256, true);

const struct suite suite_aes_ecb_128 = CIPHER_SUITE_ENTRY(
	"aes_ecb", "aes128", PSA_ALG_ECB_NO_PADDING, 128, false);
const struct suite suite_aes_ecb_192 = CIPHER_SUITE_ENTRY(
	"aes_ecb", "aes192", PSA_ALG_ECB_NO_PADDING, 192, false);
const struct suite suite_aes_ecb_256 = CIPHER_SUITE_ENTRY(
	"aes_ecb", "aes256", PSA_ALG_ECB_NO_PADDING, 256, false);

const struct suite suite_chacha20 = CIPHER_SUITE_ENTRY(
	"chacha20", "chacha20", PSA_ALG_STREAM_CIPHER, 256, true);
