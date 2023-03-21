/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include "fp_crypto.h"
#include "fp_common.h"

static void test_sha256(void)
{
	static const uint8_t input_data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

	static const uint8_t hashed_result[] = {0xBB, 0x00, 0x0D, 0xDD, 0x92, 0xA0, 0xA2, 0xA3,
						0x46, 0xF0, 0xB5, 0x31, 0xF2, 0x78, 0xAF, 0x06,
						0xE3, 0x70, 0xF8, 0x69, 0x32, 0xCC, 0xAF, 0xCC,
						0xC8, 0x92, 0xD6, 0x8D, 0x35, 0x0F, 0x80, 0xF8};

	uint8_t result_buf[FP_CRYPTO_SHA256_HASH_LEN];

	zassert_equal(sizeof(result_buf), sizeof(hashed_result),
		      "Invalid size of expected result.");
	zassert_ok(fp_crypto_sha256(result_buf, input_data, sizeof(input_data)),
		   "Error during hashing data.");
	zassert_mem_equal(result_buf, hashed_result, sizeof(hashed_result),
			  "Invalid hashing result.");
}

static void test_hmac_sha256(void)
{
	static const uint8_t input_data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xEE,
					     0x4A, 0x24, 0x83, 0x73, 0x80, 0x52, 0xE4, 0x4E, 0x9B,
					     0x2A, 0x14, 0x5E, 0x5D, 0xDF, 0xAA, 0x44, 0xB9, 0xE5,
					     0x53, 0x6A, 0xF4, 0x38, 0xE1, 0xE5, 0xC6};

	static const uint8_t aes_key[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01,
					  0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

	static const uint8_t hashed_result[] = {0x55, 0xEC, 0x5E, 0x60, 0x55, 0xAF, 0x6E, 0x92,
						0x61, 0x8B, 0x7D, 0x87, 0x10, 0xD4, 0x41, 0x37,
						0x09, 0xAB, 0x5D, 0xA2, 0x7C, 0xA2, 0x6A, 0x66,
						0xF5, 0x2E, 0x5A, 0xD4, 0xE8, 0x20, 0x90, 0x52};

	uint8_t result_buf[FP_CRYPTO_SHA256_HASH_LEN];

	zassert_equal(sizeof(result_buf), sizeof(hashed_result),
		      "Invalid size of expected result.");
	zassert_ok(fp_crypto_hmac_sha256(
		   result_buf, input_data, sizeof(input_data), aes_key, sizeof(aes_key)),
		   "Error during hashing data.");
	zassert_mem_equal(result_buf, hashed_result, sizeof(hashed_result),
			  "Invalid hashing result.");
}

static void test_aes128_ecb(void)
{
	static const uint8_t plaintext[] = {0xF3, 0x0F, 0x4E, 0x78, 0x6C, 0x59, 0xA7, 0xBB, 0xF3,
					    0x87, 0x3B, 0x5A, 0x49, 0xBA, 0x97, 0xEA};

	static const uint8_t key[] = {0xA0, 0xBA, 0xF0, 0xBB, 0x95, 0x1F, 0xF7, 0xB6, 0xCF, 0x5E,
				      0x3F, 0x45, 0x61, 0xC3, 0x32, 0x1D};

	static const uint8_t ciphertext[] = {0xAC, 0x9A, 0x16, 0xF0, 0x95, 0x3A, 0x3F, 0x22, 0x3D,
					     0xD1, 0x0C, 0xF5, 0x36, 0xE0, 0x9E, 0x9C};

	uint8_t result_buf[FP_CRYPTO_AES128_BLOCK_LEN];

	zassert_equal(sizeof(result_buf), sizeof(ciphertext), "Invalid size of expected result.");
	zassert_ok(fp_crypto_aes128_ecb_encrypt(result_buf, plaintext, key),
		   "Error during value encryption.");
	zassert_mem_equal(result_buf, ciphertext, sizeof(ciphertext), "Invalid encryption result.");

	zassert_equal(sizeof(result_buf), sizeof(plaintext), "Invalid size of expected result.");
	zassert_ok(fp_crypto_aes128_ecb_decrypt(result_buf, ciphertext, key),
		   "Error during value decryption.");
	zassert_mem_equal(result_buf, plaintext, sizeof(plaintext), "Invalid decryption result.");
}

static void test_aes128_ctr(void)
{
	static const uint8_t plaintext[] = {0x53, 0x6F, 0x6D, 0x65, 0x6F, 0x6E, 0x65, 0x27, 0x73,
					    0x20, 0x47, 0x6F, 0x6F, 0x67, 0x6C, 0x65, 0x20, 0x48,
					    0x65, 0x61, 0x64, 0x70, 0x68, 0x6F, 0x6E, 0x65};

	static const uint8_t key[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23,
				      0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

	static const uint8_t nonce[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

	static const uint8_t ciphertext[] = {0xEE, 0x4A, 0x24, 0x83, 0x73, 0x80, 0x52, 0xE4, 0x4E,
					     0x9B, 0x2A, 0x14, 0x5E, 0x5D, 0xDF, 0xAA, 0x44, 0xB9,
					     0xE5, 0x53, 0x6A, 0xF4, 0x38, 0xE1, 0xE5, 0xC6};

	zassert_equal(sizeof(plaintext), sizeof(ciphertext), "Invalid size of input buffers.");

	uint8_t result_buf[sizeof(ciphertext)];

	zassert_ok(fp_crypto_aes128_ctr_encrypt(result_buf, plaintext, sizeof(plaintext), key,
						nonce),
		   "Error during value encryption.");
	zassert_mem_equal(result_buf, ciphertext, sizeof(ciphertext), "Invalid encryption result.");

	zassert_ok(fp_crypto_aes128_ctr_decrypt(result_buf, ciphertext, sizeof(ciphertext), key,
						nonce),
		   "Error during value decryption.");
	zassert_mem_equal(result_buf, plaintext, sizeof(plaintext), "Invalid decryption result.");
}

static void test_ecdh(void)
{
	static const uint8_t bobs_private_key[] = {0x02, 0xB4, 0x37, 0xB0, 0xED, 0xD6, 0xBB, 0xD4,
						   0x29, 0x06, 0x4A, 0x4E, 0x52, 0x9F, 0xCB, 0xF1,
						   0xC4, 0x8D, 0x0D, 0x62, 0x49, 0x24, 0xD5, 0x92,
						   0x27, 0x4B, 0x7E, 0xD8, 0x11, 0x93, 0xD7, 0x63};

	static const uint8_t bobs_public_key[] = {0xF7, 0xD4, 0x96, 0xA6, 0x2E, 0xCA, 0x41, 0x63,
						  0x51, 0x54, 0x0A, 0xA3, 0x43, 0xBC, 0x69, 0x0A,
						  0x61, 0x09, 0xF5, 0x51, 0x50, 0x06, 0x66, 0xB8,
						  0x3B, 0x12, 0x51, 0xFB, 0x84, 0xFA, 0x28, 0x60,
						  0x79, 0x5E, 0xBD, 0x63, 0xD3, 0xB8, 0x83, 0x6F,
						  0x44, 0xA9, 0xA3, 0xE2, 0x8B, 0xB3, 0x40, 0x17,
						  0xE0, 0x15, 0xF5, 0x97, 0x93, 0x05, 0xD8, 0x49,
						  0xFD, 0xF8, 0xDE, 0x10, 0x12, 0x3B, 0x61, 0xD2};

	static const uint8_t alices_private_key[] = {0xD7, 0x5E, 0x54, 0xC7, 0x7D, 0x76, 0x24, 0x89,
						     0xE5, 0x7C, 0xFA, 0x92, 0x37, 0x43, 0xF1, 0x67,
						     0x77, 0xA4, 0x28, 0x3D, 0x99, 0x80, 0x0B, 0xAC,
						     0x55, 0x58, 0x48, 0x38, 0x93, 0xE5, 0xB0,
						     0x6D};

	static const uint8_t alices_public_key[] = {0x36, 0xAC, 0x68, 0x2C, 0x50, 0x82, 0x15, 0x66,
						    0x8F, 0xBE, 0xFE, 0x24, 0x7D, 0x01, 0xD5, 0xEB,
						    0x96, 0xE6, 0x31, 0x8E, 0x85, 0x5B, 0x2D, 0x64,
						    0xB5, 0x19, 0x5D, 0x38, 0xEE, 0x7E, 0x37, 0xBE,
						    0x18, 0x38, 0xC0, 0xB9, 0x48, 0xC3, 0xF7, 0x55,
						    0x20, 0xE0, 0x7E, 0x70, 0xF0, 0x72, 0x91, 0x41,
						    0x9A, 0xCE, 0x2D, 0x28, 0x14, 0x3C, 0x5A, 0xDB,
						    0x2D, 0xBD, 0x98, 0xEE, 0x3C, 0x8E, 0x4F, 0xBF};

	static const uint8_t shared_key[] = {0x9D, 0xAD, 0xE4, 0xF8, 0x6A, 0xC3, 0x48, 0x8B, 0xBA,
					     0xC2, 0xAC, 0x34, 0xB5, 0xFE, 0x68, 0xA0, 0xEE, 0x5A,
					     0x67, 0x06, 0xF5, 0x43, 0xD9, 0x06, 0x1A, 0xD5, 0x78,
					     0x89, 0x49, 0x8A, 0xE6, 0xBA};

	uint8_t bobs_result_buf[FP_CRYPTO_ECDH_SHARED_KEY_LEN];
	uint8_t alices_result_buf[FP_CRYPTO_ECDH_SHARED_KEY_LEN];

	zassert_equal(sizeof(bobs_result_buf), sizeof(shared_key),
		      "Invalid size of expected result.");
	zassert_ok(fp_crypto_ecdh_shared_secret(bobs_result_buf,
						alices_public_key,
						bobs_private_key),
		   "Error during key computing.");
	zassert_equal(sizeof(alices_result_buf), sizeof(shared_key),
		      "Invalid size of expected result.");
	zassert_ok(fp_crypto_ecdh_shared_secret(alices_result_buf,
						bobs_public_key,
						alices_private_key),
		   "Error during key computing.");
	zassert_mem_equal(bobs_result_buf, shared_key, sizeof(shared_key),
			  "Invalid key on Bob's side.");
	zassert_mem_equal(alices_result_buf, shared_key, sizeof(shared_key),
			  "Invalid key on Alice's side.");
}

static void test_aes_key_from_ecdh_shared_secret(void)
{
	static const uint8_t ecdh_shared_key[] = {0x9D, 0xAD, 0xE4, 0xF8, 0x6A, 0xC3, 0x48, 0x8B,
						  0xBA, 0xC2, 0xAC, 0x34, 0xB5, 0xFE, 0x68, 0xA0,
						  0xEE, 0x5A, 0x67, 0x06, 0xF5, 0x43, 0xD9, 0x06,
						  0x1A, 0xD5, 0x78, 0x89, 0x49, 0x8A, 0xE6, 0xBA};

	static const uint8_t aes_key[] = {0xB0, 0x7F, 0x1F, 0x17, 0xC2, 0x36, 0xCB, 0xD3, 0x35,
					  0x23, 0xC5, 0x15, 0xF3, 0x50, 0xAE, 0x57};

	uint8_t result_buf[FP_CRYPTO_AES128_KEY_LEN];

	zassert_equal(sizeof(result_buf), sizeof(aes_key), "Invalid size of expected result.");
	zassert_ok(fp_crypto_aes_key_compute(result_buf, ecdh_shared_key),
		   "Error during key computing.");
	zassert_mem_equal(result_buf, aes_key, sizeof(aes_key), "Invalid resulting key.");
}

static void test_bloom_filter(void)
{
	static const uint16_t salt = 0xC7C8;

	static const struct fp_account_key first_account_key_list[] = {
		{ .key = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0xAA, 0xBB,
			  0xCC, 0xDD, 0xEE, 0xFF} }
		};

	static const uint8_t first_bloom_filter[] = {0x02, 0x0C, 0x80, 0x2A};

	uint8_t first_result_buf[sizeof(first_bloom_filter)];

	size_t s = fp_crypto_account_key_filter_size(ARRAY_SIZE(first_account_key_list));

	zassert_equal(s, sizeof(first_bloom_filter),
		      "Invalid size of expected result.");
	zassert_ok(fp_crypto_account_key_filter(first_result_buf, first_account_key_list,
						ARRAY_SIZE(first_account_key_list), salt, NULL),
		   "Error during filter computing");
	zassert_mem_equal(first_result_buf, first_bloom_filter, sizeof(first_bloom_filter),
			  "Invalid resulting filter.");

	static const uint8_t battery_info[] = {0b00110011, 0b01000000, 0b01000000, 0b01000000};

	static const uint8_t first_bloom_filter_with_battery_info[] = {0x01, 0x01, 0x46, 0x0A};

	zassert_equal(sizeof(first_result_buf), sizeof(first_bloom_filter_with_battery_info),
		      "Invalid size of expected result.");
	zassert_ok(fp_crypto_account_key_filter(first_result_buf, first_account_key_list,
						ARRAY_SIZE(first_account_key_list), salt,
						battery_info),
		   "Error during filter computing");
	zassert_mem_equal(first_result_buf, first_bloom_filter_with_battery_info,
			  sizeof(first_bloom_filter_with_battery_info),
			  "Invalid resulting filter.");

	static const struct fp_account_key second_account_key_list[] = {
		{ .key = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0xAA, 0xBB,
			  0xCC, 0xDD, 0xEE, 0xFF} },
		{ .key = {0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66,
			  0x77, 0x77, 0x88, 0x88} }
		};

	static const uint8_t second_bloom_filter[] = {0x84, 0x4A, 0x62, 0x20, 0x8B};

	uint8_t second_result_buf[sizeof(second_bloom_filter)];

	s = fp_crypto_account_key_filter_size(ARRAY_SIZE(second_account_key_list));
	zassert_equal(s, sizeof(second_bloom_filter),
		      "Invalid size of expected result.");
	zassert_ok(fp_crypto_account_key_filter(second_result_buf, second_account_key_list,
						ARRAY_SIZE(second_account_key_list), salt, NULL),
		   "Error during filter computing");
	zassert_mem_equal(second_result_buf, second_bloom_filter, sizeof(second_bloom_filter),
			  "Invalid resulting filter.");

	static const uint8_t second_bloom_filter_with_battery_info[] = {0x46, 0x15, 0x24, 0xD0,
									0x08};

	zassert_equal(sizeof(second_result_buf), sizeof(second_bloom_filter_with_battery_info),
		      "Invalid size of expected result.");
	zassert_ok(fp_crypto_account_key_filter(second_result_buf, second_account_key_list,
						ARRAY_SIZE(second_account_key_list), salt,
						battery_info),
		   "Error during filter computing");
	zassert_mem_equal(second_result_buf, second_bloom_filter_with_battery_info,
			  sizeof(second_bloom_filter_with_battery_info),
			  "Invalid resulting filter.");
}

static void test_additional_data_packet(void)
{
	static const uint8_t input_data[] = {0x53, 0x6F, 0x6D, 0x65, 0x6F, 0x6E, 0x65, 0x27, 0x73,
					     0x20, 0x47, 0x6F, 0x6F, 0x67, 0x6C, 0x65, 0x20, 0x48,
					     0x65, 0x61, 0x64, 0x70, 0x68, 0x6F, 0x6E, 0x65};

	static const uint8_t aes_key[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01,
					  0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

	static const uint8_t nonce[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

	static const uint8_t additional_data_packet[] = {0x55, 0xEC, 0x5E, 0x60, 0x55, 0xAF, 0x6E,
							 0x92, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
							 0x06, 0x07, 0xEE, 0x4A, 0x24, 0x83, 0x73,
							 0x80, 0x52, 0xE4, 0x4E, 0x9B, 0x2A, 0x14,
							 0x5E, 0x5D, 0xDF, 0xAA, 0x44, 0xB9, 0xE5,
							 0x53, 0x6A, 0xF4, 0x38, 0xE1, 0xE5, 0xC6};

	size_t additional_data_size = sizeof(input_data) + FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN;

	zassert_equal(additional_data_size, sizeof(additional_data_packet),
		      "Invalid size of expected result.");
	uint8_t additional_data_result[additional_data_size];

	int decoded_data_size = sizeof(additional_data_packet) -
							FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN;

	zassert_equal(decoded_data_size, sizeof(input_data), "Invalid size of expected result.");
	uint8_t decoded_data_result[decoded_data_size];

	zassert_ok(fp_crypto_additional_data_encode(additional_data_result, input_data,
						    sizeof(input_data), aes_key, nonce),
		   "Error during data encoding.");
	zassert_mem_equal(additional_data_result, additional_data_packet,
			  sizeof(additional_data_packet), "Invalid encoding result.");

	zassert_ok(fp_crypto_additional_data_decode(decoded_data_result, additional_data_packet,
						    sizeof(additional_data_packet), aes_key),
		   "Error during data decoding.");
	zassert_mem_equal(decoded_data_result, input_data, sizeof(input_data),
			  "Invalid decoding result.");

	/* Test data integrity check. */
	additional_data_result[2] += 1;
	zassert_not_equal(fp_crypto_additional_data_decode(decoded_data_result,
							   additional_data_result,
							   sizeof(additional_data_result), aes_key),
			  0, "Expected error during decoding non-integral packet.");
}

void test_main(void)
{
	ztest_test_suite(fast_pair_crypto_tests,
			 ztest_unit_test(test_sha256),
			 ztest_unit_test(test_hmac_sha256),
			 ztest_unit_test(test_aes128_ecb),
			 ztest_unit_test(test_aes128_ctr),
			 ztest_unit_test(test_ecdh),
			 ztest_unit_test(test_aes_key_from_ecdh_shared_secret),
			 ztest_unit_test(test_bloom_filter),
			 ztest_unit_test(test_additional_data_packet)
			 );

	ztest_run_test_suite(fast_pair_crypto_tests);
}
