/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include "common_test.h"

/**@brief ChaCHa Poly test vectors can be found in RFC 7539 document.
 *
 * https://tools.ietf.org/html/rfc7539
*/

/* Multiple used ChaCha Poly test vectors. */
const char chachapoly_plain_114[] = {
	"4c616469657320616e642047656e746c656d656e206f662074686520636c617373206f6620"
	"2739393a204966204920636f756c64206f6666657220796f75206f6e6c79206f6e65207469"
	"7020666f7220746865206675747572652c2073756e73637265656e20776f756c6420626520"
	"69742e"
};
const char chachapoly_cipher_114[] = {
	"d31a8d34648e60db7b86afbc53ef7ec2a4aded51296e08fea9e2b5a736ee62d63dbea45e8c"
	"a9671282fafb69da92728b1a71de0a9e060b2905d6a5b67ecd3b3692ddbd7f2d778b8c9803"
	"aee328091b58fab324e4fad675945585808b4831d7bc3ff4def08e4b7a9de576d26586cec6"
	"4b6116"
};
const char chachapoly_key[] = {
	"808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f"
};
const char chachapoly_ad[] = { "50515253c0c1c2c3c4c5c6c7" };
const char chachapoly_nonce[] = { "070000004041424344454647" };
const char chachapoly_mac[] = { "1ae10b594f09e26a7e902ecbd0600691" };
const char chachapoly_invalid_key[] = {
	"908182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f"
};
const char chachapoly_invalid_mac[] = { "2ae10b594f09e26a7e902ecbd0600691" };

/* ChaCha Poly - RFC 7539 - section A.5 */
ITEM_REGISTER(test_vector_aead_chachapoly_data,
	      test_vector_aead_t test_vector_aes_aead_chachapoly_full_1) = {
	.mode = MBEDTLS_MODE_CHACHAPOLY,
	.id = MBEDTLS_CIPHER_ID_CHACHA20,
	.expected_err_code = 0,
	.crypt_expected_result = EXPECTED_TO_PASS,
	.mac_expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("ChaChaPoly message_len=265 ad_len=12"),
	.p_plaintext =
		"496e7465726e65742d4472616674732061726520647261667420646f63756d656e7473"
		"2076616c696420666f722061206d6178696d756d206f6620736978206d6f6e74687320"
		"616e64206d617920626520757064617465642c207265706c616365642c206f72206f62"
		"736f6c65746564206279206f7468657220646f63756d656e747320617420616e792074"
		"696d652e20497420697320696e617070726f70726961746520746f2075736520496e74"
		"65726e65742d447261667473206173207265666572656e6365206d6174657269616c20"
		"6f7220746f2063697465207468656d206f74686572207468616e206173202fe2809c77"
		"6f726b20696e2070726f67726573732e2fe2809d",
	.p_ciphertext =
		"64a0861575861af460f062c79be643bd5e805cfd345cf389f108670ac76c8cb24c6cfc"
		"18755d43eea09ee94e382d26b0bdb7b73c321b0100d4f03b7f355894cf332f830e710b"
		"97ce98c8a84abd0b948114ad176e008d33bd60f982b1ff37c8559797a06ef4f0ef61c1"
		"86324e2b3506383606907b6a7c02b0f9f6157b53c867e4b9166c767b804d46a59b5216"
		"cde7a4e99040c5a40433225ee282a1b0a06c523eaf4534d7f83fa1155b0047718cbc54"
		"6a0d072b04b3564eea1b422273f548271a0bb2316053fa76991955ebd63159434ecebb"
		"4e466dae5a1073a6727627097a1049e617d91d361094fa68f0ff77987130305beaba2e"
		"da04df997b714d6c6f2c29a6ad5cb4022b02709b",
	.p_key =
		"1c9240a5eb55d38af333888604f6b5f0473917c1402b80099dca5cbc207075c0",
	.p_ad = "f33388860000000000004e91",
	.p_nonce = "000000000102030405060708",
	.p_mac = "eead9d67890cbb22392336fea1851f38"
};

/* ChaCha Poly - RFC 7539 - section 2.8.2 */
ITEM_REGISTER(test_vector_aead_chachapoly_data,
	      test_vector_aead_t test_vector_aes_aead_chachapoly_full_2) = {
	.mode = MBEDTLS_MODE_CHACHAPOLY,
	.id = MBEDTLS_CIPHER_ID_CHACHA20,
	.expected_err_code = 0,
	.crypt_expected_result = EXPECTED_TO_PASS,
	.mac_expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("ChaChaPoly message_len=114 ad_len=12"),
	.p_plaintext = chachapoly_plain_114,
	.p_ciphertext = chachapoly_cipher_114,
	.p_key = chachapoly_key,
	.p_ad = chachapoly_ad,
	.p_nonce = chachapoly_nonce,
	.p_mac = chachapoly_mac
};

/* ChaCha Poly - Based on RFC 7539 - section 2.8.2 */
/* Update: See RFC 7539 2.8. Quote concerning lengths: "Arbitrary length additional authenticated data (AAD)" */
/* Therefore this should be expected to be valid, not invalid. */
ITEM_REGISTER(test_vector_aead_chachapoly_simple_data,
	      test_vector_aead_t test_vector_aes_aead_chachapoly_ad0_valid) = {
	.mode = MBEDTLS_MODE_CHACHAPOLY,
	.id = MBEDTLS_CIPHER_ID_CHACHA20,
	.expected_err_code = 0,
	.crypt_expected_result = EXPECTED_TO_PASS,
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("ChaChaPoly Valid ad_len=0"),
	.p_plaintext = chachapoly_plain_114,
	.p_ciphertext = chachapoly_cipher_114,
	.p_key = chachapoly_key,
	.p_ad = "",
	.p_nonce = chachapoly_nonce,
	.p_mac = chachapoly_mac
};

/* ChaCha Poly - Based on RFC 7539 - section 2.8.2 */
ITEM_REGISTER(test_vector_aead_chachapoly_data,
	      test_vector_aead_t test_vector_aes_aead_chachapoly_inv_nonce) = {
	.mode = MBEDTLS_MODE_CHACHAPOLY,
	.id = MBEDTLS_CIPHER_ID_CHACHA20,
	.expected_err_code = MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA,
	.crypt_expected_result = EXPECTED_TO_FAIL,
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("ChaChaPoly Invalid nonce_len=12"),
	.p_plaintext = chachapoly_plain_114,
	.p_ciphertext = chachapoly_cipher_114,
	.p_key = chachapoly_key,
	.p_ad = chachapoly_ad,
	.p_nonce = "0000000001020304050607",
	.p_mac = chachapoly_mac
};

/* ChaCha Poly - Based on RFC 7539 - section 2.8.2 */
ITEM_REGISTER(test_vector_aead_chachapoly_data,
	      test_vector_aead_t test_vector_aes_aead_chachapoly_inv_mac_len) = {
	.mode = MBEDTLS_MODE_CHACHAPOLY,
	.id = MBEDTLS_CIPHER_ID_CHACHA20,
	.expected_err_code = MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA,
	.crypt_expected_result = EXPECTED_TO_FAIL,
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("ChaChaPoly Invalid mac_len=15"),
	.p_plaintext = chachapoly_plain_114,
	.p_ciphertext = chachapoly_cipher_114,
	.p_key = chachapoly_key,
	.p_ad = chachapoly_ad,
	.p_nonce = chachapoly_nonce,
	.p_mac = "1ae10b594f09e26a7e902ecbd06006"
};

/* ChaCha Poly - Based on RFC 7539 - section 2.8.2 */
ITEM_REGISTER(
	test_vector_aead_chachapoly_simple_data,
	test_vector_aead_t test_vector_aes_aead_chachapoly_inv_key_encrypt) = {
	.mode = MBEDTLS_MODE_CHACHAPOLY,
	.id = MBEDTLS_CIPHER_ID_CHACHA20,
	.expected_err_code = 0,
	.crypt_expected_result = EXPECTED_TO_FAIL,
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("ChaChaPoly Encrypt Invalid key"),
	.p_plaintext = chachapoly_plain_114,
	.p_ciphertext = chachapoly_cipher_114,
	.p_key = chachapoly_invalid_key,
	.p_ad = chachapoly_ad,
	.p_nonce = chachapoly_nonce,
	.p_mac = chachapoly_mac
};

/* ChaCha Poly - Based on RFC 7539 - section 2.8.2 */
ITEM_REGISTER(
	test_vector_aead_chachapoly_simple_data,
	test_vector_aead_t test_vector_aes_aead_chachapoly_inv_key_decrypt) = {
	.mode = MBEDTLS_MODE_CHACHAPOLY,
	.id = MBEDTLS_CIPHER_ID_CHACHA20,
	.expected_err_code = MBEDTLS_ERR_CIPHER_AUTH_FAILED,
	.crypt_expected_result = EXPECTED_TO_FAIL,
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name = TV_NAME("ChaChaPoly Decrypt Invalid key"),
	.p_plaintext = chachapoly_plain_114,
	.p_ciphertext = chachapoly_cipher_114,
	.p_key = chachapoly_invalid_key,
	.p_ad = chachapoly_ad,
	.p_nonce = chachapoly_nonce,
	.p_mac = chachapoly_mac
};

/* ChaCha Poly - Based on RFC 7539 - section 2.8.2 */
ITEM_REGISTER(
	test_vector_aead_chachapoly_simple_data,
	test_vector_aead_t test_vector_aes_aead_chachapoly_inv_mac_encrypt) = {
	.mode = MBEDTLS_MODE_CHACHAPOLY,
	.id = MBEDTLS_CIPHER_ID_CHACHA20,
	.expected_err_code = 0,
	.crypt_expected_result = EXPECTED_TO_PASS,
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("ChaChaPoly Encrypt Invalid mac"),
	.p_plaintext = chachapoly_plain_114,
	.p_ciphertext = chachapoly_cipher_114,
	.p_key = chachapoly_key,
	.p_ad = chachapoly_ad,
	.p_nonce = chachapoly_nonce,
	.p_mac = chachapoly_invalid_mac
};

/* ChaCha Poly - Based on RFC 7539 - section 2.8.2 */
ITEM_REGISTER(
	test_vector_aead_chachapoly_simple_data,
	test_vector_aead_t test_vector_aes_aead_chachapoly_inv_mac_decrypt) = {
	.mode = MBEDTLS_MODE_CHACHAPOLY,
	.id = MBEDTLS_CIPHER_ID_CHACHA20,
	.expected_err_code = MBEDTLS_ERR_CIPHER_AUTH_FAILED,
	.crypt_expected_result = EXPECTED_TO_FAIL,
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name = TV_NAME("ChaChaPoly Decrypt Invalid mac"),
	.p_plaintext = chachapoly_plain_114,
	.p_ciphertext = chachapoly_cipher_114,
	.p_key = chachapoly_key,
	.p_ad = chachapoly_ad,
	.p_nonce = chachapoly_nonce,
	.p_mac = chachapoly_invalid_mac
};
