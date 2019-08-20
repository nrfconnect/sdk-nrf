/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>

#include "common_test.h"

#include <mbedtls/ecdh.h>

/**@brief ECDH test vectors can be found on NIST web pages.
 *
 * http://read.pudn.com/downloads168/doc/772358/TestVectorsforSEC%201-gec2.pdf
 * https://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/Component-Testing
 */
#if defined(CONFIG_MBEDTLS_ECP_DP_SECP160R1_ENABLED)

/* ECDH - SECP160R1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_secp160r1_random) = {
	.curve_type = MBEDTLS_ECP_DP_SECP160R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp160r1 valid")
};

/* ECDH - GEC 2: Test Vectors for SEC 1 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_full,
	      test_vector_ecdh_t test_vector_ecdh_full_secp160r1_1) = {
	.curve_type = MBEDTLS_ECP_DP_SECP160R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp160r1"),
	.p_initiator_priv = "aa374ffc3ce144e6b073307972cb6d57b2a4e982",
	.p_responder_priv = "45fb58a92a17ad4b15101c66e74f277e2b460866",
	.p_initiator_publ_x = "51b4496fecc406ed0e75a24a3c03206251419dc0",
	.p_initiator_publ_y = "c28dcb4b73a514b468d793894f381ccc1756aa6c",
	.p_responder_publ_x = "49b41e0e9c0369c2328739d90f63d56707c6e5bc",
	.p_responder_publ_y = "26e008b567015ed96d232a03111c3edc0e9c8f83",
	.p_expected_shared_secret = "ca7c0f8c3ffa87a96e1b74ac8e6af594347bb40a"
};

#endif /* MBEDTLS_ECP_DP_SECP160R1_ENABLED */

#if defined(CONFIG_MBEDTLS_ECP_DP_SECP160R2_ENABLED)

/* ECDH - SECP160R2 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_secp160r2_random) = {
	.curve_type = MBEDTLS_ECP_DP_SECP160R2,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp160r2 valid")
};

#endif /* MBEDTLS_ECP_DP_SECP160R2_ENABLED */

#if defined(CONFIG_MBEDTLS_ECP_DP_SECP192R1_ENABLED)

/* ECDH - SECP192R1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_secp192r1_random) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp192r1 valid")
};

/* ECDH - NIST CAVS 14.1 P-192 - Count 0 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp192r1_1) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp192r1 valid 1"),
	.p_responder_priv = "f17d3fea367b74d340851ca4270dcb24c271f445bed9d527",
	.p_initiator_publ_x = "42ea6dd9969dd2a61fea1aac7f8e98edcc896c6e55857cc0",
	.p_initiator_publ_y = "dfbe5d7c61fac88b11811bde328e8a0d12bf01a9d204b523",
	.p_expected_shared_secret =
		"803d8ab2e5b6e6fca715737c3a82f7ce3c783124f6d51cd0"
};

/* ECDH - NIST CAVS 14.1 P-192 - Count 1 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp192r1_2) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp192r1 valid 2"),
	.p_responder_priv = "56e853349d96fe4c442448dacb7cf92bb7a95dcf574a9bd5",
	.p_initiator_publ_x = "deb5712fa027ac8d2f22c455ccb73a91e17b6512b5e030e7",
	.p_initiator_publ_y = "7e2690a02cc9b28708431a29fb54b87b1f0c14e011ac2125",
	.p_expected_shared_secret =
		"c208847568b98835d7312cef1f97f7aa298283152313c29d"
};

/* ECDH - NIST CAVS 14.1 P-192 - Count 2 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp192r1_3) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp192r1 valid 3"),
	.p_responder_priv = "c6ef61fe12e80bf56f2d3f7d0bb757394519906d55500949",
	.p_initiator_publ_x = "4edaa8efc5a0f40f843663ec5815e7762dddc008e663c20f",
	.p_initiator_publ_y = "0a9f8dc67a3e60ef6d64b522185d03df1fc0adfd42478279",
	.p_expected_shared_secret =
		"87229107047a3b611920d6e3b2c0c89bea4f49412260b8dd"
};

/* ECDH - NIST CAVS 14.1 P-192 - Count 3 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp192r1_4) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp192r1 valid 4"),
	.p_responder_priv = "e6747b9c23ba7044f38ff7e62c35e4038920f5a0163d3cda",
	.p_initiator_publ_x = "8887c276edeed3e9e866b46d58d895c73fbd80b63e382e88",
	.p_initiator_publ_y = "04c5097ba6645e16206cfb70f7052655947dd44a17f1f9d5",
	.p_expected_shared_secret =
		"eec0bed8fc55e1feddc82158fd6dc0d48a4d796aaf47d46c"
};

/* ECDH - NIST CAVS 14.1 P-192 - Count 4 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp192r1_5) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp192r1 valid 5"),
	.p_responder_priv = "beabedd0154a1afcfc85d52181c10f5eb47adc51f655047d",
	.p_initiator_publ_x = "0d045f30254adc1fcefa8a5b1f31bf4e739dd327cd18d594",
	.p_initiator_publ_y = "542c314e41427c08278a08ce8d7305f3b5b849c72d8aff73",
	.p_expected_shared_secret =
		"716e743b1b37a2cd8479f0a3d5a74c10ba2599be18d7e2f4"
};

/* ECDH - NIST CAVS 14.1 P-192 - Invalid public key test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp192r1_inv_q) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = MBEDTLS_ERR_ECP_INVALID_KEY,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp192r1 invalid public key"),
	.p_responder_priv = "beabedd0154a1afcfc85d52181c10f5eb47adc51f655047d",
	.p_initiator_publ_x = "1d045f30254adc1fcefa8a5b1f31bf4e739dd327cd18d594",
	.p_initiator_publ_y = "542c314e41427c08278a08ce8d7305f3b5b849c72d8aff73",
	.p_expected_shared_secret =
		"716e743b1b37a2cd8479f0a3d5a74c10ba2599be18d7e2f4"
};

/* ECDH - NIST CAVS 14.1 P-192 - Invalid private key test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp192r1_inv_d) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp192r1 invalid private key"),
	.p_responder_priv = "ceabedd0154a1afcfc85d52181c10f5eb47adc51f655047d",
	.p_initiator_publ_x = "0d045f30254adc1fcefa8a5b1f31bf4e739dd327cd18d594",
	.p_initiator_publ_y = "542c314e41427c08278a08ce8d7305f3b5b849c72d8aff73",
	.p_expected_shared_secret =
		"716e743b1b37a2cd8479f0a3d5a74c10ba2599be18d7e2f4"
};

/* ECDH - NIST CAVS 14.1 P-192 - Invalid shared secret test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp192r1_inv_ss) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp192r1 invalid shared secret"),
	.p_responder_priv = "beabedd0154a1afcfc85d52181c10f5eb47adc51f655047d",
	.p_initiator_publ_x = "0d045f30254adc1fcefa8a5b1f31bf4e739dd327cd18d594",
	.p_initiator_publ_y = "542c314e41427c08278a08ce8d7305f3b5b849c72d8aff73",
	.p_expected_shared_secret =
		"816e743b1b37a2cd8479f0a3d5a74c10ba2599be18d7e2f4"
};
#endif /* MBEDTLS_ECP_DP_SECP192R1_ENABLED */

#if defined(CONFIG_MBEDTLS_ECP_DP_SECP224R1_ENABLED)

/* ECDH - SECP224R1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_secp224r1_random) = {
	.curve_type = MBEDTLS_ECP_DP_SECP224R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp224r1 valid")
};

/* ECDH - NIST CAVS 14.1 P-224 - Count 0 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp224r1_1) = {
	.curve_type = MBEDTLS_ECP_DP_SECP224R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp224r1 valid 1"),
	.p_responder_priv =
		"8346a60fc6f293ca5a0d2af68ba71d1dd389e5e40837942df3e43cbd",
	.p_initiator_publ_x =
		"af33cd0629bc7e996320a3f40368f74de8704fa37b8fab69abaae280",
	.p_initiator_publ_y =
		"882092ccbba7930f419a8a4f9bb16978bbc3838729992559a6f2e2d7",
	.p_expected_shared_secret =
		"7d96f9a3bd3c05cf5cc37feb8b9d5209d5c2597464dec3e9983743e8"
};

/* ECDH - NIST CAVS 14.1 P-224 - Count 1 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp224r1_2) = {
	.curve_type = MBEDTLS_ECP_DP_SECP224R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp224r1 valid 2"),
	.p_responder_priv =
		"043cb216f4b72cdf7629d63720a54aee0c99eb32d74477dac0c2f73d",
	.p_initiator_publ_x =
		"13bfcd4f8e9442393cab8fb46b9f0566c226b22b37076976f0617a46",
	.p_initiator_publ_y =
		"eeb2427529b288c63c2f8963c1e473df2fca6caa90d52e2f8db56dd4",
	.p_expected_shared_secret =
		"ee93ce06b89ff72009e858c68eb708e7bc79ee0300f73bed69bbca09"
};

/* ECDH - NIST CAVS 14.1 P-224 - Count 2 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp224r1_3) = {
	.curve_type = MBEDTLS_ECP_DP_SECP224R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp224r1 valid 3"),
	.p_responder_priv =
		"5ad0dd6dbabb4f3c2ea5fe32e561b2ca55081486df2c7c15c9622b08",
	.p_initiator_publ_x =
		"756dd806b9d9c34d899691ecb45b771af468ec004486a0fdd283411e",
	.p_initiator_publ_y =
		"4d02c2ca617bb2c5d9613f25dd72413d229fd2901513aa29504eeefb",
	.p_expected_shared_secret =
		"3fcc01e34d4449da2a974b23fc36f9566754259d39149790cfa1ebd3"
};

/* ECDH - NIST CAVS 14.1 P-224 - Count 3 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp224r1_4) = {
	.curve_type = MBEDTLS_ECP_DP_SECP224R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp224r1 valid 4"),
	.p_responder_priv =
		"0aa6ff55a5d820efcb4e7d10b845ea3c9f9bc5dff86106db85318e22",
	.p_initiator_publ_x =
		"0f537bf1c1122c55656d25e8aa8417e0b44b1526ae0523144f9921c4",
	.p_initiator_publ_y =
		"f79b26d30e491a773696cc2c79b4f0596bc5b9eebaf394d162fb8684",
	.p_expected_shared_secret =
		"49129628b23afcef48139a3f6f59ff5e9811aa746aa4ff33c24bb940"
};

/* ECDH - NIST CAVS 14.1 P-224 - Count 4 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp224r1_5) = {
	.curve_type = MBEDTLS_ECP_DP_SECP224R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp224r1 valid 5"),
	.p_responder_priv =
		"efe6e6e25affaf54c98d002abbc6328da159405a1b752e32dc23950a",
	.p_initiator_publ_x =
		"2b3631d2b06179b3174a100f7f57131eeea8947be0786c3dc64b2239",
	.p_initiator_publ_y =
		"83de29ae3dad31adc0236c6de7f14561ca2ea083c5270c78a2e6cbc0",
	.p_expected_shared_secret =
		"fcdc69a40501d308a6839653a8f04309ec00233949522902ffa5eac6"
};

/* ECDH - NIST CAVS 14.1 P-224 - Invalid public key test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp224r1_inv_q) = {
	.curve_type = MBEDTLS_ECP_DP_SECP224R1,
	.expected_err_code = MBEDTLS_ERR_ECP_INVALID_KEY,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp224r1 invalid public key"),
	.p_responder_priv =
		"efe6e6e25affaf54c98d002abbc6328da159405a1b752e32dc23950a",
	.p_initiator_publ_x =
		"3b3631d2b06179b3174a100f7f57131eeea8947be0786c3dc64b2239",
	.p_initiator_publ_y =
		"83de29ae3dad31adc0236c6de7f14561ca2ea083c5270c78a2e6cbc0",
	.p_expected_shared_secret =
		"fcdc69a40501d308a6839653a8f04309ec00233949522902ffa5eac6"
};

/* ECDH - NIST CAVS 14.1 P-224 - Invalid private key test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp224r1_inv_d) = {
	.curve_type = MBEDTLS_ECP_DP_SECP224R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp224r1 invalid private key"),
	.p_responder_priv =
		"ffe6e6e25affaf54c98d002abbc6328da159405a1b752e32dc23950a",
	.p_initiator_publ_x =
		"2b3631d2b06179b3174a100f7f57131eeea8947be0786c3dc64b2239",
	.p_initiator_publ_y =
		"83de29ae3dad31adc0236c6de7f14561ca2ea083c5270c78a2e6cbc0",
	.p_expected_shared_secret =
		"fcdc69a40501d308a6839653a8f04309ec00233949522902ffa5eac6"
};

/* ECDH - NIST CAVS 14.1 P-224 - Invalid shared secret test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp224r1_inv_ss) = {
	.curve_type = MBEDTLS_ECP_DP_SECP224R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp224r1 invalid shared secret"),
	.p_responder_priv =
		"efe6e6e25affaf54c98d002abbc6328da159405a1b752e32dc23950a",
	.p_initiator_publ_x =
		"2b3631d2b06179b3174a100f7f57131eeea8947be0786c3dc64b2239",
	.p_initiator_publ_y =
		"83de29ae3dad31adc0236c6de7f14561ca2ea083c5270c78a2e6cbc0",
	.p_expected_shared_secret =
		"0cdc69a40501d308a6839653a8f04309ec00233949522902ffa5eac6"
};
#endif /* MBEDTLS_ECP_DP_SECP224R1_ENABLED */

#if defined(CONFIG_MBEDTLS_ECP_DP_SECP256R1_ENABLED)

/* ECDH - SECP256R1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_secp256r1_random) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp256r1 valid")
};

/* ECDH - RFC 5903 256-Bit Random ECP Group */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_full,
	      test_vector_ecdh_t test_vector_ecdh_secp256r1_full) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp256r1"),
	.p_initiator_priv =
		"C88F01F510D9AC3F70A292DAA2316DE544E9AAB8AFE84049C62A9C57862D1433",
	.p_responder_priv =
		"C6EF9C5D78AE012A011164ACB397CE2088685D8F06BF9BE0B283AB46476BEE53",
	.p_initiator_publ_x =
		"DAD0B65394221CF9B051E1FECA5787D098DFE637FC90B9EF945D0C3772581180",
	.p_initiator_publ_y =
		"5271A0461CDB8252D61F1C456FA3E59AB1F45B33ACCF5F58389E0577B8990BB3",
	.p_responder_publ_x =
		"D12DFB5289C8D4F81208B70270398C342296970A0BCCB74C736FC7554494BF63",
	.p_responder_publ_y =
		"56FBF3CA366CC23E8157854C13C58D6AAC23F046ADA30F8353E74F33039872AB",
	.p_expected_shared_secret =
		"D6840F6B42F6EDAFD13116E0E12565202FEF8E9ECE7DCE03812464D04B9442DE"
};

/* ECDH - NIST CAVS 14.1 P-256 - Count 0 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp256r1_1) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp256r1 valid 1"),
	.p_responder_priv =
		"7d7dc5f71eb29ddaf80d6214632eeae03d9058af1fb6d22ed80badb62bc1a534",
	.p_initiator_publ_x =
		"700c48f77f56584c5cc632ca65640db91b6bacce3a4df6b42ce7cc838833d287",
	.p_initiator_publ_y =
		"db71e509e3fd9b060ddb20ba5c51dcc5948d46fbf640dfe0441782cab85fa4ac",
	.p_expected_shared_secret =
		"46fc62106420ff012e54a434fbdd2d25ccc5852060561e68040dd7778997bd7b"
};

/* ECDH - NIST CAVS 14.1 P-256 - Count 1 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp256r1_2) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp256r1 valid 2"),
	.p_responder_priv =
		"38f65d6dce47676044d58ce5139582d568f64bb16098d179dbab07741dd5caf5",
	.p_initiator_publ_x =
		"809f04289c64348c01515eb03d5ce7ac1a8cb9498f5caa50197e58d43a86a7ae",
	.p_initiator_publ_y =
		"b29d84e811197f25eba8f5194092cb6ff440e26d4421011372461f579271cda3",
	.p_expected_shared_secret =
		"057d636096cb80b67a8c038c890e887d1adfa4195e9b3ce241c8a778c59cda67"
};

/* ECDH - NIST CAVS 14.1 P-256 - Count 2 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp256r1_3) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp256r1 valid 3"),
	.p_responder_priv =
		"1accfaf1b97712b85a6f54b148985a1bdc4c9bec0bd258cad4b3d603f49f32c8",
	.p_initiator_publ_x =
		"a2339c12d4a03c33546de533268b4ad667debf458b464d77443636440ee7fec3",
	.p_initiator_publ_y =
		"ef48a3ab26e20220bcda2c1851076839dae88eae962869a497bf73cb66faf536",
	.p_expected_shared_secret =
		"2d457b78b4614132477618a5b077965ec90730a8c81a1c75d6d4ec68005d67ec"
};

/* ECDH - NIST CAVS 14.1 P-256 - Count 3 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp256r1_4) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp256r1 valid 4"),
	.p_responder_priv =
		"207c43a79bfee03db6f4b944f53d2fb76cc49ef1c9c4d34d51b6c65c4db6932d",
	.p_initiator_publ_x =
		"df3989b9fa55495719b3cf46dccd28b5153f7808191dd518eff0c3cff2b705ed",
	.p_initiator_publ_y =
		"422294ff46003429d739a33206c8752552c8ba54a270defc06e221e0feaf6ac4",
	.p_expected_shared_secret =
		"96441259534b80f6aee3d287a6bb17b5094dd4277d9e294f8fe73e48bf2a0024"
};

/* ECDH - NIST CAVS 14.1 P-256 - Count 4 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp256r1_5) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp256r1 valid 5"),
	.p_responder_priv =
		"59137e38152350b195c9718d39673d519838055ad908dd4757152fd8255c09bf",
	.p_initiator_publ_x =
		"41192d2813e79561e6a1d6f53c8bc1a433a199c835e141b05a74a97b0faeb922",
	.p_initiator_publ_y =
		"1af98cc45e98a7e041b01cf35f462b7562281351c8ebf3ffa02e33a0722a1328",
	.p_expected_shared_secret =
		"19d44c8d63e8e8dd12c22a87b8cd4ece27acdde04dbf47f7f27537a6999a8e62"
};

/* ECDH - NIST CAVS 14.1 P-256 - Invalid public key test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp256r1_inv_q) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_err_code = MBEDTLS_ERR_ECP_INVALID_KEY,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp256r1 invalid public key"),
	.p_responder_priv =
		"59137e38152350b195c9718d39673d519838055ad908dd4757152fd8255c09bf",
	.p_initiator_publ_x =
		"51192d2813e79561e6a1d6f53c8bc1a433a199c835e141b05a74a97b0faeb922",
	.p_initiator_publ_y =
		"1af98cc45e98a7e041b01cf35f462b7562281351c8ebf3ffa02e33a0722a1328",
	.p_expected_shared_secret =
		"19d44c8d63e8e8dd12c22a87b8cd4ece27acdde04dbf47f7f27537a6999a8e62"
};

/* ECDH - NIST CAVS 14.1 P-256 - Invalid private key test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp256r1_inv_d) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp256r1 invalid private key"),
	.p_responder_priv =
		"69137e38152350b195c9718d39673d519838055ad908dd4757152fd8255c09bf",
	.p_initiator_publ_x =
		"41192d2813e79561e6a1d6f53c8bc1a433a199c835e141b05a74a97b0faeb922",
	.p_initiator_publ_y =
		"1af98cc45e98a7e041b01cf35f462b7562281351c8ebf3ffa02e33a0722a1328",
	.p_expected_shared_secret =
		"19d44c8d63e8e8dd12c22a87b8cd4ece27acdde04dbf47f7f27537a6999a8e62"
};

/* ECDH - NIST CAVS 14.1 P-256 - Invalid shared secret test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp256r1_inv_ss) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp256r1 invalid shared secret"),
	.p_responder_priv =
		"59137e38152350b195c9718d39673d519838055ad908dd4757152fd8255c09bf",
	.p_initiator_publ_x =
		"41192d2813e79561e6a1d6f53c8bc1a433a199c835e141b05a74a97b0faeb922",
	.p_initiator_publ_y =
		"1af98cc45e98a7e041b01cf35f462b7562281351c8ebf3ffa02e33a0722a1328",
	.p_expected_shared_secret =
		"29d44c8d63e8e8dd12c22a87b8cd4ece27acdde04dbf47f7f27537a6999a8e62"
};
#endif /* MBEDTLS_ECP_DP_SECP256R1 */

#if defined(CONFIG_MBEDTLS_ECP_DP_SECP384R1_ENABLED)

/* ECDH - SECP384R1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_secp384r1_random) = {
	.curve_type = MBEDTLS_ECP_DP_SECP384R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp384r1 valid")
};

/* ECDH - RFC 5903 384-Bit Random ECP Group */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_full,
	      test_vector_ecdh_t test_vector_ecdh_secp384r1_full) = {
	.curve_type = MBEDTLS_ECP_DP_SECP384R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp384r1"),
	.p_initiator_priv =
		"099F3C7034D4A2C699884D73A375A67F7624EF7C6B3C0F160647B6"
		"7414DCE655E35B538041E649EE3FAEF896783AB194",
	.p_responder_priv =
		"41CB0779B4BDB85D47846725FBEC3C9430FAB46CC8DC5060855CC9"
		"BDA0AA2942E0308312916B8ED2960E4BD55A7448FC",
	.p_initiator_publ_x =
		"667842D7D180AC2CDE6F74F37551F55755C7645C20EF73E31634"
		"FE72B4C55EE6DE3AC808ACB4BDB4C88732AEE95F41AA",
	.p_initiator_publ_y =
		"9482ED1FC0EEB9CAFC4984625CCFC23F65032149E0E144ADA024"
		"181535A0F38EEB9FCFF3C2C947DAE69B4C634573A81C",
	.p_responder_publ_x =
		"E558DBEF53EECDE3D3FCCFC1AEA08A89A987475D12FD950D83CF"
		"A41732BC509D0D1AC43A0336DEF96FDA41D0774A3571",
	.p_responder_publ_y =
		"DCFBEC7AACF3196472169E838430367F66EEBE3C6E70C416DD5F"
		"0C68759DD1FFF83FA40142209DFF5EAAD96DB9E6386C",
	.p_expected_shared_secret =
		"11187331C279962D93D604243FD592CB9D0A926F422E47187521287E7156C5C4D60313"
		"5569B9E9D09CF5D4A270F59746"
};

/* ECDH - NIST CAVS 14.1 P-384 - Count 0 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp384r1_1) = {
	.curve_type = MBEDTLS_ECP_DP_SECP384R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp384r1 valid 1"),
	.p_responder_priv =
		"3cc3122a68f0d95027ad38c067916ba0eb8c38894d22e1b15618b6"
		"818a661774ad463b205da88cf699ab4d43c9cf98a1",
	.p_initiator_publ_x =
		"a7c76b970c3b5fe8b05d2838ae04ab47697b9eaf52e764592efd"
		"a27fe7513272734466b400091adbf2d68c58e0c50066",
	.p_initiator_publ_y =
		"ac68f19f2e1cb879aed43a9969b91a0839c4c38a49749b661efe"
		"df243451915ed0905a32b060992b468c64766fc8437a",
	.p_expected_shared_secret =
		"5f9d29dc5e31a163060356213669c8ce132e22f57c9a04f40ba7fcead493b457e5621e"
		"766c40a2e3d4d6a04b25e533f1"
};

/* ECDH - NIST CAVS 14.1 P-384 - Count 1 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp384r1_2) = {
	.curve_type = MBEDTLS_ECP_DP_SECP384R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp384r1 valid 2"),
	.p_responder_priv =
		"92860c21bde06165f8e900c687f8ef0a05d14f290b3f07d8b3a8cc"
		"6404366e5d5119cd6d03fb12dc58e89f13df9cd783",
	.p_initiator_publ_x =
		"30f43fcf2b6b00de53f624f1543090681839717d53c7c955d1d6"
		"9efaf0349b7363acb447240101cbb3af6641ce4b88e0",
	.p_initiator_publ_y =
		"25e46c0c54f0162a77efcc27b6ea792002ae2ba82714299c8608"
		"57a68153ab62e525ec0530d81b5aa15897981e858757",
	.p_expected_shared_secret =
		"a23742a2c267d7425fda94b93f93bbcc24791ac51cd8fd501a238d40812f4cbfc59aac"
		"9520d758cf789c76300c69d2ff"
};

/* ECDH - NIST CAVS 14.1 P-384 - Count 2 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp384r1_3) = {
	.curve_type = MBEDTLS_ECP_DP_SECP384R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp384r1 valid 3"),
	.p_responder_priv =
		"12cf6a223a72352543830f3f18530d5cb37f26880a0b294482c8a8"
		"ef8afad09aa78b7dc2f2789a78c66af5d1cc553853",
	.p_initiator_publ_x =
		"1aefbfa2c6c8c855a1a216774550b79a24cda37607bb1f7cc906"
		"650ee4b3816d68f6a9c75da6e4242cebfb6652f65180",
	.p_initiator_publ_y =
		"419d28b723ebadb7658fcebb9ad9b7adea674f1da3dc6b6397b5"
		"5da0f61a3eddacb4acdb14441cb214b04a0844c02fa3",
	.p_expected_shared_secret =
		"3d2e640f350805eed1ff43b40a72b2abed0a518bcebe8f2d15b111b6773223da3c3489"
		"121db173d414b5bd5ad7153435"
};

/* ECDH - NIST CAVS 14.1 P-384 - Count 3 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp384r1_4) = {
	.curve_type = MBEDTLS_ECP_DP_SECP384R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp384r1 valid 4"),
	.p_responder_priv =
		"8dd48063a3a058c334b5cc7a4ce07d02e5ee6d8f1f3c51a1600962"
		"cbab462690ae3cd974fb39e40b0e843daa0fd32de1",
	.p_initiator_publ_x =
		"8bc089326ec55b9cf59b34f0eb754d93596ca290fcb3444c83d4"
		"de3a5607037ec397683f8cef07eab2fe357eae36c449",
	.p_initiator_publ_y =
		"d9d16ce8ac85b3f1e94568521aae534e67139e310ec72693526a"
		"a2e927b5b322c95a1a033c229cb6770c957cd3148dd7",
	.p_expected_shared_secret =
		"6a42cfc392aba0bfd3d17b7ccf062b91fc09bbf3417612d02a90bdde62ae40c54bb2e5"
		"6e167d6b70db670097eb8db854"
};

/* ECDH - NIST CAVS 14.1 P-384 - Count 4 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp384r1_5) = {
	.curve_type = MBEDTLS_ECP_DP_SECP384R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp384r1 valid 5"),
	.p_responder_priv =
		"84ece6cc3429309bd5b23e959793ed2b111ec5cb43b6c18085fcae"
		"a9efa0685d98a6262ee0d330ee250bc8a67d0e733f",
	.p_initiator_publ_x =
		"eb952e2d9ac0c20c6cc48fb225c2ad154f53c8750b003fd3b4ed"
		"8ed1dc0defac61bcdde02a2bcfee7067d75d342ed2b0",
	.p_initiator_publ_y =
		"f1828205baece82d1b267d0d7ff2f9c9e15b69a72df47058a97f"
		"3891005d1fb38858f5603de840e591dfa4f6e7d489e1",
	.p_expected_shared_secret =
		"ce7ba454d4412729a32bb833a2d1fd2ae612d4667c3a900e069214818613447df8c611"
		"de66da200db7c375cf913e4405"
};

/* ECDH - NIST CAVS 14.1 P-384 - Invalid public key test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp384r1_inv_q) = {
	.curve_type = MBEDTLS_ECP_DP_SECP384R1,
	.expected_err_code = MBEDTLS_ERR_ECP_INVALID_KEY,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp384r1 invalid public key"),
	.p_responder_priv =
		"84ece6cc3429309bd5b23e959793ed2b111ec5cb43b6c18085fcae"
		"a9efa0685d98a6262ee0d330ee250bc8a67d0e733f",
	.p_initiator_publ_x =
		"fb952e2d9ac0c20c6cc48fb225c2ad154f53c8750b003fd3b4ed"
		"8ed1dc0defac61bcdde02a2bcfee7067d75d342ed2b0",
	.p_initiator_publ_y =
		"f1828205baece82d1b267d0d7ff2f9c9e15b69a72df47058a97f"
		"3891005d1fb38858f5603de840e591dfa4f6e7d489e1",
	.p_expected_shared_secret =
		"ce7ba454d4412729a32bb833a2d1fd2ae612d4667c3a900e069214818613447df8c611"
		"de66da200db7c375cf913e4405"
};

/* ECDH - NIST CAVS 14.1 P-384 - Invalid private key test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp384r1_inv_d) = {
	.curve_type = MBEDTLS_ECP_DP_SECP384R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp384r1 invalid private key"),
	.p_responder_priv =
		"94ece6cc3429309bd5b23e959793ed2b111ec5cb43b6c18085fcae"
		"a9efa0685d98a6262ee0d330ee250bc8a67d0e733f",
	.p_initiator_publ_x =
		"eb952e2d9ac0c20c6cc48fb225c2ad154f53c8750b003fd3b4ed"
		"8ed1dc0defac61bcdde02a2bcfee7067d75d342ed2b0",
	.p_initiator_publ_y =
		"f1828205baece82d1b267d0d7ff2f9c9e15b69a72df47058a97f"
		"3891005d1fb38858f5603de840e591dfa4f6e7d489e1",
	.p_expected_shared_secret =
		"ce7ba454d4412729a32bb833a2d1fd2ae612d4667c3a900e069214818613447df8c611"
		"de66da200db7c375cf913e4405"
};

/* ECDH - NIST CAVS 14.1 P-384 - Invalid shared secret test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp384r1_inv_ss) = {
	.curve_type = MBEDTLS_ECP_DP_SECP384R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp384r1 invalid shared secret"),
	.p_responder_priv =
		"84ece6cc3429309bd5b23e959793ed2b111ec5cb43b6c18085fcae"
		"a9efa0685d98a6262ee0d330ee250bc8a67d0e733f",
	.p_initiator_publ_x =
		"eb952e2d9ac0c20c6cc48fb225c2ad154f53c8750b003fd3b4ed"
		"8ed1dc0defac61bcdde02a2bcfee7067d75d342ed2b0",
	.p_initiator_publ_y =
		"f1828205baece82d1b267d0d7ff2f9c9e15b69a72df47058a97f"
		"3891005d1fb38858f5603de840e591dfa4f6e7d489e1",
	.p_expected_shared_secret =
		"de7ba454d4412729a32bb833a2d1fd2ae612d4667c3a900e069214818613447df8c611"
		"de66da200db7c375cf913e4405"
};
#endif /* MBEDTLS_ECP_DP_SECP384R1_ENABLED */

#if defined(CONFIG_MBEDTLS_ECP_DP_SECP521R1_ENABLED)

/* ECDH - SECP521R1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_secp521r1_random) = {
	.curve_type = MBEDTLS_ECP_DP_SECP521R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp521r1 valid")
};

/* ECDH - RFC 5903 521-Bit Random ECP Group */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_full,
	      test_vector_ecdh_t test_vector_ecdh_secp521r1_full) = {
	.curve_type = MBEDTLS_ECP_DP_SECP521R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp521r1"),
	.p_initiator_priv =
		"0037ADE9319A89F4DABDB3EF411AACCCA5123C61ACAB57B5393DCE47608172A095AA85"
		"A30FE1C2952C6771D937BA9777F5957B2639BAB072462F68C27A57382D4A52",
	.p_responder_priv =
		"0145BA99A847AF43793FDD0E872E7CDFA16BE30FDC780F97BCCC3F078380201E9C677D"
		"600B343757A3BDBF2A3163E4C2F869CCA7458AA4A4EFFC311F5CB151685EB9",
	.p_initiator_publ_x =
		"0015417E84DBF28C0AD3C278713349DC7DF153C897A1891BD98BAB4357C9ECBEE1E3BF"
		"42E00B8E380AEAE57C2D107564941885942AF5A7F4601723C4195D176CED3E",
	.p_initiator_publ_y =
		"017CAE20B6641D2EEB695786D8C946146239D099E18E1D5A514C739D7CB4A10AD8A788"
		"015AC405D7799DC75E7B7D5B6CF2261A6A7F1507438BF01BEB6CA3926F9582",
	.p_responder_publ_x =
		"00D0B3975AC4B799F5BEA16D5E13E9AF971D5E9B984C9F39728B5E5739735A219B97C3"
		"56436ADC6E95BB0352F6BE64A6C2912D4EF2D0433CED2B6171640012D9460F",
	.p_responder_publ_y =
		"015C68226383956E3BD066E797B623C27CE0EAC2F551A10C2C724D9852077B87220B65"
		"36C5C408A1D2AEBB8E86D678AE49CB57091F4732296579AB44FCD17F0FC56A",
	.p_expected_shared_secret =
		"01144C7D79AE6956BC8EDB8E7C787C4521CB086FA64407F97894E5E6B2D79B04D1427E"
		"73CA4BAA240A34786859810C06B3C715A3A8CC3151F2BEE417996D19F3DDEA"
};

/* ECDH - NIST CAVS 14.1 P-521 - Count 0 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp521r1_1) = {
	.curve_type = MBEDTLS_ECP_DP_SECP521R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp521r1 valid 1"),
	.p_responder_priv =
		"017eecc07ab4b329068fba65e56a1f8890aa935e57134ae0ffcce802735151f4eac656"
		"4f6ee9974c5e6887a1fefee5743ae2241bfeb95d5ce31ddcb6f9edb4d6fc47",
	.p_initiator_publ_x =
		"00685a48e86c79f0f0875f7bc18d25eb5fc8c0b07e5da4f4370f3a9490340854334b1e"
		"1b87fa395464c60626124a4e70d0f785601d37c09870ebf176666877a2046d",
	.p_initiator_publ_y =
		"01ba52c56fc8776d9e8f5db4f0cc27636d0b741bbe05400697942e80b739884a83bde9"
		"9e0f6716939e632bc8986fa18dccd443a348b6c3e522497955a4f3c302f676",
	.p_expected_shared_secret =
		"005fc70477c3e63bc3954bd0df3ea0d1f41ee21746ed95fc5e1fdf90930d5e136672d7"
		"2cc770742d1711c3c3a4c334a0ad9759436a4d3c5bf6e74b9578fac148c831"
};

/* ECDH - NIST CAVS 14.1 P-521 - Count 1 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp521r1_2) = {
	.curve_type = MBEDTLS_ECP_DP_SECP521R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp521r1 valid 2"),
	.p_responder_priv =
		"00816f19c1fb10ef94d4a1d81c156ec3d1de08b66761f03f06ee4bb9dcebbbfe1eaa1e"
		"d49a6a990838d8ed318c14d74cc872f95d05d07ad50f621ceb620cd905cfb8",
	.p_initiator_publ_x =
		"01df277c152108349bc34d539ee0cf06b24f5d3500677b4445453ccc21409453aafb8a"
		"72a0be9ebe54d12270aa51b3ab7f316aa5e74a951c5e53f74cd95fc29aee7a",
	.p_initiator_publ_y =
		"013d52f33a9f3c14384d1587fa8abe7aed74bc33749ad9c570b471776422c7d4505d9b"
		"0a96b3bfac041e4c6a6990ae7f700e5b4a6640229112deafa0cd8bb0d089b0",
	.p_expected_shared_secret =
		"000b3920ac830ade812c8f96805da2236e002acbbf13596a9ab254d44d0e91b6255ebf"
		"1229f366fb5a05c5884ef46032c26d42189273ca4efa4c3db6bd12a6853759"
};

/* ECDH - NIST CAVS 14.1 P-521 - Count 2 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp521r1_3) = {
	.curve_type = MBEDTLS_ECP_DP_SECP521R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp521r1 valid 3"),
	.p_responder_priv =
		"012f2e0c6d9e9d117ceb9723bced02eb3d4eebf5feeaf8ee0113ccd8057b13ddd416e0"
		"b74280c2d0ba8ed291c443bc1b141caf8afb3a71f97f57c225c03e1e4d42b0",
	.p_initiator_publ_x =
		"0092db3142564d27a5f0006f819908fba1b85038a5bc2509906a497daac67fd7aee0fc"
		"2daba4e4334eeaef0e0019204b471cd88024f82115d8149cc0cf4f7ce1a4d5",
	.p_initiator_publ_y =
		"016bad0623f517b158d9881841d2571efbad63f85cbe2e581960c5d670601a67602726"
		"75a548996217e4ab2b8ebce31d71fca63fcc3c08e91c1d8edd91cf6fe845f8",
	.p_expected_shared_secret =
		"006b380a6e95679277cfee4e8353bf96ef2a1ebdd060749f2f046fe571053740bbcc9a"
		"0b55790bc9ab56c3208aa05ddf746a10a3ad694daae00d980d944aabc6a08f"
};

/* ECDH - NIST CAVS 14.1 P-521 - Count 3 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp521r1_4) = {
	.curve_type = MBEDTLS_ECP_DP_SECP521R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp521r1 valid 4"),
	.p_responder_priv =
		"00e548a79d8b05f923b9825d11b656f222e8cb98b0f89de1d317184dc5a698f7c71161"
		"ee7dc11cd31f4f4f8ae3a981e1a3e78bdebb97d7c204b9261b4ef92e0918e0",
	.p_initiator_publ_x =
		"00fdd40d9e9d974027cb3bae682162eac1328ad61bc4353c45bf5afe76bf607d2894c8"
		"cce23695d920f2464fda4773d4693be4b3773584691bdb0329b7f4c86cc299",
	.p_initiator_publ_y =
		"0034ceac6a3fef1c3e1c494bfe8d872b183832219a7e14da414d4e3474573671ec19b0"
		"33be831b915435905925b44947c592959945b4eb7c951c3b9c8cf52530ba23",
	.p_expected_shared_secret =
		"00fbbcd0b8d05331fef6086f22a6cce4d35724ab7a2f49dd8458d0bfd57a0b8b70f246"
		"c17c4468c076874b0dff7a0336823b19e98bf1cec05e4beffb0591f97713c6"
};

/* ECDH - NIST CAVS 14.1 P-521 - Count 4 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp521r1_5) = {
	.curve_type = MBEDTLS_ECP_DP_SECP521R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp521r1 valid 5"),
	.p_responder_priv =
		"01c8aae94bb10b8ca4f7be577b4fb32bb2381032c4942c24fc2d753e7cc5e47b483389"
		"d9f3b956d20ee9001b1eef9f23545f72c5602140046839e963313c3decc864",
	.p_initiator_publ_x =
		"0098d99dee0816550e84dbfced7e88137fddcf581a725a455021115fe49f8dc3cf233c"
		"d9ea0e6f039dc7919da973cdceaca205da39e0bd98c8062536c47f258f44b5",
	.p_initiator_publ_y =
		"00cd225c8797371be0c4297d2b457740100c774141d8f214c23b61aa2b6cd4806b9b70"
		"722aa4965fb622f42b7391e27e5ec21c5679c5b06b59127372997d421adc1e",
	.p_expected_shared_secret =
		"0145cfa38f25943516c96a5fd4bfebb2f645d10520117aa51971eff442808a23b4e23c"
		"187e639ff928c3725fbd1c0c2ad0d4aeb207bc1a6fb6cb6d467888dc044b3c"
};

/* ECDH - NIST CAVS 14.1 P-521 - Invalid public key test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp521r1_inv_q) = {
	.curve_type = MBEDTLS_ECP_DP_SECP521R1,
	.expected_err_code = MBEDTLS_ERR_ECP_INVALID_KEY,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp521r1 invalid public key"),
	.p_responder_priv =
		"01c8aae94bb10b8ca4f7be577b4fb32bb2381032c4942c24fc2d753e7cc5e47b483389"
		"d9f3b956d20ee9001b1eef9f23545f72c5602140046839e963313c3decc864",
	.p_initiator_publ_x =
		"00a8d99dee0816550e84dbfced7e88137fddcf581a725a455021115fe49f8dc3cf233c"
		"d9ea0e6f039dc7919da973cdceaca205da39e0bd98c8062536c47f258f44b5",
	.p_initiator_publ_y =
		"00cd225c8797371be0c4297d2b457740100c774141d8f214c23b61aa2b6cd4806b9b70"
		"722aa4965fb622f42b7391e27e5ec21c5679c5b06b59127372997d421adc1e",
	.p_expected_shared_secret =
		"0145cfa38f25943516c96a5fd4bfebb2f645d10520117aa51971eff442808a23b4e23c"
		"187e639ff928c3725fbd1c0c2ad0d4aeb207bc1a6fb6cb6d467888dc044b3c"
};

/* ECDH - NIST CAVS 14.1 P-521 - Invalid private key test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp521r1_inv_d) = {
	.curve_type = MBEDTLS_ECP_DP_SECP521R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp521r1 invalid private key"),
	.p_responder_priv =
		"01d8aae94bb10b8ca4f7be577b4fb32bb2381032c4942c24fc2d753e7cc5e47b483389"
		"d9f3b956d20ee9001b1eef9f23545f72c5602140046839e963313c3decc864",
	.p_initiator_publ_x =
		"0098d99dee0816550e84dbfced7e88137fddcf581a725a455021115fe49f8dc3cf233c"
		"d9ea0e6f039dc7919da973cdceaca205da39e0bd98c8062536c47f258f44b5",
	.p_initiator_publ_y =
		"00cd225c8797371be0c4297d2b457740100c774141d8f214c23b61aa2b6cd4806b9b70"
		"722aa4965fb622f42b7391e27e5ec21c5679c5b06b59127372997d421adc1e",
	.p_expected_shared_secret =
		"0145cfa38f25943516c96a5fd4bfebb2f645d10520117aa51971eff442808a23b4e23c"
		"187e639ff928c3725fbd1c0c2ad0d4aeb207bc1a6fb6cb6d467888dc044b3c"
};

/* ECDH - NIST CAVS 14.1 P-521 - Invalid shared secret test case */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_simple,
	      test_vector_ecdh_t test_vector_ecdh_secp521r1_inv_ss) = {
	.curve_type = MBEDTLS_ECP_DP_SECP521R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("secp521r1 invalid shared secret"),
	.p_responder_priv =
		"01c8aae94bb10b8ca4f7be577b4fb32bb2381032c4942c24fc2d753e7cc5e47b483389"
		"d9f3b956d20ee9001b1eef9f23545f72c5602140046839e963313c3decc864",
	.p_initiator_publ_x =
		"0098d99dee0816550e84dbfced7e88137fddcf581a725a455021115fe49f8dc3cf233c"
		"d9ea0e6f039dc7919da973cdceaca205da39e0bd98c8062536c47f258f44b5",
	.p_initiator_publ_y =
		"00cd225c8797371be0c4297d2b457740100c774141d8f214c23b61aa2b6cd4806b9b70"
		"722aa4965fb622f42b7391e27e5ec21c5679c5b06b59127372997d421adc1e",
	.p_expected_shared_secret =
		"0155cfa38f25943516c96a5fd4bfebb2f645d10520117aa51971eff442808a23b4e23c"
		"187e639ff928c3725fbd1c0c2ad0d4aeb207bc1a6fb6cb6d467888dc044b3c"
};
#endif /* MBEDTLS_ECP_DP_SECP521R1_ENABLED */

#if defined(CONFIG_MBEDTLS_ECP_DP_CURVE25519_ENABLED)

/* NOTE: mbedtls expects a different endianness on the private keys than the one provided by RFC 7748. */
/* Instead of making a special case in the source, the keys are swapped here. */
/* Needs a review. */
/* Should either: */
/*      * Keep it as it is now */
/*      * Delete "unfixed" version (--> can not easily track vector data to RFC 7748) */
/*      * Delete "fixed" version (--> have to concat + reverse in code) */
#define CURVE25519_REVERSED_VECTORS
#if !defined(CURVE25519_REVERSED_VECTORS)
const char ecdh_curve25519_initiator_priv[] = {
	"77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2"
	"a"
}; /* Original.
					 */
const char ecdh_curve25519_responder_priv[] = {
	"5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0e"
	"b"
}; /* Original.
					 */
const char ecdh_curve25519_initiator_publ_x[] = {
	"8520f0098930a754748b7ddcb43ef75a"
}; /* Original. */
const char ecdh_curve25519_initiator_publ_y[] = {
	"0dbf3a0d26381af4eba4a98eaa9b4e6a"
}; /* Original. */
const char ecdh_curve25519_responder_publ_x[] = {
	"de9edb7d7b7dc1b4d35b61c2ece43537"
}; /* Original. */
const char ecdh_curve25519_responder_publ_y[] = {
	"3f8343c85b78674dadfc7e146f882b4f"
}; /* Original. */
const char ecdh_curve25519_expected_shared_secret[] = {
	"4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e16174"
	"2"
}; /* Original.
					 */
#else
const char ecdh_curve25519_initiator_priv[] = {
	"2a2cb91da5fb77b12a99c0eb872f4cdf4566b25172c1163c7da518730a6d077"
	"7"
}; /* Reversed.
					 */
const char ecdh_curve25519_responder_priv[] = {
	"ebe088ff278b2f1cfdb6182629b13b6fe60e80838b7fe1794b8a4a627e08ab5"
	"d"
}; /* Reversed.
					 */
const char ecdh_curve25519_initiator_publ_x[] = {
	"6a4e9baa8ea9a4ebf41a38260d3abf0d5af73eb4dc7d8b7454a7308909f0208"
	"5"
}; /* Reversed
														 concatenation
														 of X
														 and
														 Y.
													   */
const char ecdh_curve25519_initiator_publ_y[] = { "" }; /* Cleared. */
const char ecdh_curve25519_responder_publ_x[] = {
	"4f2b886f147efcad4d67785bc843833f3735e4ecc2615bd3b4c17d7b7ddb9ed"
	"e"
}; /* Reversed
														 concatenation
														 of X
														 and
														 Y.
													   */
const char ecdh_curve25519_responder_publ_y[] = { "" }; /* Cleared. */
const char ecdh_curve25519_expected_shared_secret[] = {
	"4217161e3c9bf076339ed147c9217ee0250f3580f43b8e72e12dcea45b9d5d4"
	"a"
}; /* Reversed.
					 */
#endif /* CURVE25519_REVERSED_VECTORS */

/* ECDH - RFC 7748 - 6.1. Curve25519 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_full,
	      test_vector_ecdh_t test_vector_ecdh_curve25519_full_1) = {
	.curve_type = MBEDTLS_ECP_DP_CURVE25519,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("curve25519"),
	.p_initiator_priv = ecdh_curve25519_initiator_priv,
	.p_responder_priv = ecdh_curve25519_responder_priv,
	.p_initiator_publ_x = ecdh_curve25519_initiator_publ_x,
	.p_initiator_publ_y = ecdh_curve25519_initiator_publ_y,
	.p_responder_publ_x = ecdh_curve25519_responder_publ_x,
	.p_responder_publ_y = ecdh_curve25519_responder_publ_y,
	.p_expected_shared_secret = ecdh_curve25519_expected_shared_secret
};

/* ECDH - Based on RFC 7748 - 6.1. Curve25519 Test Vector */
ITEM_REGISTER(
	test_vector_ecdh_data_deterministic_full,
	test_vector_ecdh_t test_vector_ecdh_curve25519_full_inv_priv_key) = {
	.curve_type = MBEDTLS_ECP_DP_CURVE25519,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("curve25519 invalid private key"),
	.p_initiator_priv =
		"87076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a",
	.p_responder_priv =
		"6dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb",
	.p_initiator_publ_x = ecdh_curve25519_initiator_publ_x,
	.p_initiator_publ_y = ecdh_curve25519_initiator_publ_y,
	.p_responder_publ_x = ecdh_curve25519_responder_publ_x,
	.p_responder_publ_y = ecdh_curve25519_responder_publ_y,
	.p_expected_shared_secret = ecdh_curve25519_expected_shared_secret
};

/* ECDH - Based on RFC 7748 - 6.1. Curve25519 Test Vector */
ITEM_REGISTER(
	test_vector_ecdh_data_deterministic_full,
	test_vector_ecdh_t test_vector_ecdh_curve25519_full_inv_publ_x_key) = {
	.curve_type = MBEDTLS_ECP_DP_CURVE25519,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("curve25519 invalid public X key"),
	.p_initiator_priv = ecdh_curve25519_initiator_priv,
	.p_responder_priv = ecdh_curve25519_responder_priv,
	.p_initiator_publ_x = "9520f0098930a754748b7ddcb43ef75a",
	.p_initiator_publ_y = ecdh_curve25519_initiator_publ_y,
	.p_responder_publ_x = ecdh_curve25519_responder_publ_x,
	.p_responder_publ_y = ecdh_curve25519_responder_publ_y,
	.p_expected_shared_secret = ecdh_curve25519_expected_shared_secret
};

#endif /* MBEDTLS_ECP_DP_CURVE25519_ENABLED */
#if defined(CONFIG_MBEDTLS_ECP_DP_SECP160K1_ENABLED)

/* ECDH - SECP160K1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_secp160k1_random) = {
	.curve_type = MBEDTLS_ECP_DP_SECP160K1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp160k1 valid")
};

#endif /* MBEDTLS_ECP_DP_SECP160K1_ENABLED */

#if defined(CONFIG_MBEDTLS_ECP_DP_SECP192K1_ENABLED)

/* ECDH - SECP192K1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_secp192k1_random) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192K1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp192k1 valid")
};
#endif /* MBEDTLS_ECP_DP_SECP192K1_ENABLED */

#if defined(CONFIG_MBEDTLS_ECP_DP_SECP224K1_ENABLED)

/* ECDH - SECP224K1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_secp224k1_random) = {
	.curve_type = MBEDTLS_ECP_DP_SECP224K1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp224k1 valid")
};
#endif /* MBEDTLS_ECP_DP_SECP224K1_ENABLED */

#if defined(CONFIG_MBEDTLS_ECP_DP_SECP256K1_ENABLED)

/* ECDH - SECP256K1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_secp256k1_random) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256K1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("secp256k1 valid")
};
#endif /* MBEDTLS_ECP_DP_SECP256K1_ENABLED */

#if !defined(CONFIG_CRYPTO_LONG_RUNNING_VECTORS_DISABLE)
#if defined(CONFIG_MBEDTLS_ECP_DP_BP256R1_ENABLED)

/* ECDH - BP256R1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_bp256r1_random) = {
	.curve_type = MBEDTLS_ECP_DP_BP256R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("bp256r1 valid")
};

/* ECDH - RFC 7027 - A.1 - Curve brainpoolP256r1 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_full,
	      test_vector_ecdh_t test_vector_ecdh_bp256r1_full) = {
	.curve_type = MBEDTLS_ECP_DP_BP256R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("bp256r1"),
	.p_initiator_priv =
		"81db1ee100150ff2ea338d708271be38300cb54241d79950f77b063039804f1d",
	.p_responder_priv =
		"55e40bc41e37e3e2ad25c3c6654511ffa8474a91a0032087593852d3e7d76bd3",
	.p_initiator_publ_x =
		"44106e913f92bc02a1705d9953a8414db95e1aaa49e81d9e85f929a8e3100be5",
	.p_initiator_publ_y =
		"8ab4846f11caccb73ce49cbdd120f5a900a69fd32c272223f789ef10eb089bdc",
	.p_responder_publ_x =
		"8d2d688c6cf93e1160ad04cc4429117dc2c41825e1e9fca0addd34e6f1b39f7b",
	.p_responder_publ_y =
		"990c57520812be512641e47034832106bc7d3e8dd0e4c7f1136d7006547cec6a",
	.p_expected_shared_secret =
		"89afc39d41d3b327814b80940b042590f96556ec91e6ae7939bce31f3a18bf2b"
};

#endif /* MBEDTLS_ECP_DP_BP256R1_ENABLED */

#if defined(CONFIG_MBEDTLS_ECP_DP_BP384R1_ENABLED)

/* ECDH - BP384R1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_bp384r1_random) = {
	.curve_type = MBEDTLS_ECP_DP_BP384R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("bp384r1 valid")
};

/* ECDH - RFC 7027 - A.2 - Curve brainpoolP384r1 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_full,
	      test_vector_ecdh_t test_vector_ecdh_bp384r1_full) = {
	.curve_type = MBEDTLS_ECP_DP_BP384R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("bp384r1"),
	.p_initiator_priv =
		"1e20f5e048a5886f1f157c74e91bde2b98c8b52d58e5003d57053f"
		"c4b0bd65d6f15eb5d1ee1610df870795143627d042",
	.p_responder_priv =
		"032640bc6003c59260f7250c3db58ce647f98e1260acce4acda3dd"
		"869f74e01f8ba5e0324309db6a9831497abac96670",
	.p_initiator_publ_x =
		"68b665dd91c195800650cdd363c625f4e742e8134667b767b1b4"
		"76793588f885ab698c852d4a6e77a252d6380fcaf068",
	.p_initiator_publ_y =
		"55bc91a39c9ec01dee36017b7d673a931236d2f1f5c83942d049"
		"e3fa20607493e0d038ff2fd30c2ab67d15c85f7faa59",
	.p_responder_publ_x =
		"4d44326f269a597a5b58bba565da5556ed7fd9a8a9eb76c25f46"
		"db69d19dc8ce6ad18e404b15738b2086df37e71d1eb4",
	.p_responder_publ_y =
		"62d692136de56cbe93bf5fa3188ef58bc8a3a0ec6c1e151a2103"
		"8a42e9185329b5b275903d192f8d4e1f32fe9cc78c48",
	.p_expected_shared_secret =
		"0bd9d3a7ea0b3d519d09d8e48d0785fb744a6b355e6304bc51c229fbbce239bbadf640"
		"3715c35d4fb2a5444f575d4f42"
};

#endif /* MBEDTLS_ECP_DP_BP384R1_ENABLED */

#if defined(CONFIG_MBEDTLS_ECP_DP_BP512R1_ENABLED)

/* ECDH - BP512R1 - Random test vectors */
ITEM_REGISTER(test_vector_ecdh_data_random,
	      test_vector_ecdh_t test_vector_ecdh_bp512r1_random) = {
	.curve_type = MBEDTLS_ECP_DP_BP512R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("bp512r1 valid")
};

/* ECDH - RFC 7027 - A.3 - Curve brainpoolP512r1 */
ITEM_REGISTER(test_vector_ecdh_data_deterministic_full,
	      test_vector_ecdh_t test_vector_ecdh_bp512r1_full) = {
	.curve_type = MBEDTLS_ECP_DP_BP512R1,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("bp512r1"),
	.p_initiator_priv =
		"16302ff0dbbb5a8d733dab7141c1b45acbc8715939677f6a56850a38bd87bd59b09e80"
		"279609ff333eb9d4c061231fb26f92eeb04982a5f1d1764cad57665422",
	.p_responder_priv =
		"230e18e1bcc88a362fa54e4ea3902009292f7f8033624fd471b5d8ace49d12cfabbc19"
		"963dab8e2f1eba00bffb29e4d72d13f2224562f405cb80503666b25429",
	.p_initiator_publ_x =
		"0a420517e406aac0acdce90fcd71487718d3b953efd7fbec5f7f27e28c6149999397e9"
		"1e029e06457db2d3e640668b392c2a7e737a7f0bf04436d11640fd09fd",
	.p_initiator_publ_y =
		"72e6882e8db28aad36237cd25d580db23783961c8dc52dfa2ec138ad472a0fcef3887c"
		"f62b623b2a87de5c588301ea3e5fc269b373b60724f5e82a6ad147fde7",
	.p_responder_publ_x =
		"9d45f66de5d67e2e6db6e93a59ce0bb48106097ff78a081de781cdb31fce8ccbaaea8d"
		"d4320c4119f1e9cd437a2eab3731fa9668ab268d871deda55a5473199f",
	.p_responder_publ_y =
		"2fdc313095bcdd5fb3a91636f07a959c8e86b5636a1e930e8396049cb481961d365cc1"
		"1453a06c719835475b12cb52fc3c383bce35e27ef194512b71876285fa",
	.p_expected_shared_secret =
		"a7927098655f1f9976fa50a9d566865dc530331846381c87256baf3226244b76d36403"
		"c024d7bbf0aa0803eaff405d3d24f11a9b5c0bef679fe1454b21c4cd1f"
};

#endif /* MBEDTLS_ECP_DP_BP512R1_ENABLED */
#endif /* CONFIG_CRYPTO_LONG_RUNNING_VECTORS_DISABLE */
