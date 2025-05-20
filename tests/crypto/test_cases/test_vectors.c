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
#include <mbedtls/md.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/ecjpake.h>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ccm.h>

/*
 *  Use this file to only run a small set of test vectors.
 *  This file will only be relevant if the correct option is set with cmake:
 *  cmake -GNinja -DBOARD=nrf52840dk/nrf52840 -DREDUCED_TEST_SUITE=1 ..
 */

#if defined(MBEDTLS_CCM_C)

/* AES CCM - Custom test vector 1 - Invalid behavior test for AES plaintext and AD. */
ITEM_REGISTER(test_vector_aead_ccm_data,
	      test_vector_aead_t test_vector_aes_ccm_128_inv_c18) = {
	.mode = MBEDTLS_MODE_CCM,
	.id = MBEDTLS_CIPHER_ID_AES,
	.ccm_star = false,
	.expected_err_code = 0,
	.crypt_expected_result = EXPECTED_TO_PASS,
	.mac_expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CCM 128 message_len=0 ad_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key = "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf",
	.p_ad = "",
	.p_nonce = "00000003020100a0a1a2a3a4a5",
	.p_mac = "f48122034d40c898"
};

/* AES CCM - Custom test vector 2 - Invalid behavior test for AES buffer authenticated decryption. */
ITEM_REGISTER(test_vector_aead_ccm_simple_data,
	      test_vector_aead_t test_vector_aes_ccm_128_inv_c19) = {
	.mode = MBEDTLS_MODE_CCM,
	.id = MBEDTLS_CIPHER_ID_AES,
	.ccm_star = false,
	.expected_err_code = MBEDTLS_ERR_CIPHER_AUTH_FAILED,
	.crypt_expected_result = EXPECTED_TO_FAIL,
	.mac_expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name = TV_NAME("CCM 128 Decrypt Invalid ciphertext"),
	.p_plaintext = "08090a0b0c0d0e0f101112131415161718191a1b1c1d1e",
	.p_ciphertext = "688c979a61c663d2f066d0c2c0f989806d5f6b61dac384",
	.p_key = "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf",
	.p_ad = "0001020304050607",
	.p_nonce = "00000003020100a0a1a2a3a4a5",
	.p_mac = "17e8d12cfdf926e0"
};

/* AES CCM - Custom test vector 3 - Invalid behavior test for AES buffer authenticated decryption. */
ITEM_REGISTER(test_vector_aead_ccm_simple_data,
	      test_vector_aead_t test_vector_aes_ccm_128_inv_c20) = {
	.mode = MBEDTLS_MODE_CCM,
	.id = MBEDTLS_CIPHER_ID_AES,
	.ccm_star = false,
	.expected_err_code = MBEDTLS_ERR_CIPHER_AUTH_FAILED,
	.crypt_expected_result =
		EXPECTED_TO_FAIL, /* Generated plaintext will be incorrect. */
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name = TV_NAME("CCM 128 Decrypt Invalid MAC"),
	.p_plaintext = "08090a0b0c0d0e0f101112131415161718191a1b1c1d1e",
	.p_ciphertext = "588c979a61c663d2f066d0c2c0f989806d5f6b61dac384",
	.p_key = "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf",
	.p_ad = "0001020304050607",
	.p_nonce = "00000003020100a0a1a2a3a4a5",
	.p_mac = "27e8d12cfdf926e0"
};

/* AES CCM STAR - Custom Test vector. */
ITEM_REGISTER(test_vector_aead_ccm_simple_data,
	      test_vector_aead_t test_vector_aes_ccm_star_128_inv_c7) = {
	.mode = MBEDTLS_MODE_CCM,
	.id = MBEDTLS_CIPHER_ID_AES,
	.ccm_star = true,
	.expected_err_code = MBEDTLS_ERR_CCM_AUTH_FAILED,
	.crypt_expected_result = EXPECTED_TO_FAIL,
	.mac_expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name = TV_NAME("CCM STAR 128 Decrypt Invalid ciphertext"),
	.p_plaintext = "34cb14f841ef56495779d46b21978724",
	.p_ciphertext = "a6b87289284ed8779e98a5bf55d16f00",
	.p_key = "d1fa7145ecd7a327ca3a8b58cd1147e6",
	.p_ad = "04ebed593e86388a",
	.p_nonce = "01f04f8873ea675d98a43a4e06",
	.p_mac = "a3fd8b8dae862dc5"
};

/* AES CCM STAR - Custom Test vector. */
ITEM_REGISTER(test_vector_aead_ccm_simple_data,
	      test_vector_aead_t test_vector_aes_ccm_star_128_inv_c5) = {
	.mode = MBEDTLS_MODE_CCM,
	.id = MBEDTLS_CIPHER_ID_AES,
	.ccm_star = true,
	.expected_err_code = MBEDTLS_ERR_CCM_AUTH_FAILED,
	.crypt_expected_result = EXPECTED_TO_FAIL,
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name =
		TV_NAME("CCM STAR 128 Decrypt with invalid AES key"),
	.p_plaintext = "34cb14f841ef56495779d46b21978724",
	.p_ciphertext = "96b87289284ed8779e98a5bf55d16f00",
	.p_key = "e1fa7145ecd7a327ca3a8b58cd1147e6",
	.p_ad = "04ebed593e86388a",
	.p_nonce = "01f04f8873ea675d98a43a4e06",
	.p_mac = "a3fd8b8dae862dc5"
};

/* AES CCM STAR - Custom Test vector. */
ITEM_REGISTER(test_vector_aead_ccm_simple_data,
	      test_vector_aead_t test_vector_aes_ccm_star_128_inv_c6) = {
	.mode = MBEDTLS_MODE_CCM,
	.id = MBEDTLS_CIPHER_ID_AES,
	.ccm_star = true,
	.expected_err_code = MBEDTLS_ERR_CCM_AUTH_FAILED,
	.crypt_expected_result = EXPECTED_TO_FAIL,
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name = TV_NAME("CCM STAR 128 Decrypt Invalid MAC"),
	.p_plaintext = "34cb14f841ef56495779d46b21978724",
	.p_ciphertext = "96b87289284ed8779e98a5bf55d16f00",
	.p_key = "d1fa7145ecd7a327ca3a8b58cd1147e6",
	.p_ad = "04ebed593e86388a",
	.p_nonce = "01f04f8873ea675d98a43a4e06",
	.p_mac = "b3fd8b8dae862dc5"
};

#endif

#if defined(MBEDTLS_CHACHAPOLY_C)
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
#endif

#if defined(MBEDTLS_AES_C)
#if defined(MBEDTLS_GCM_C)
/* AES GCM - NIST CAVS 14.0 Decrypt with keysize 256 - Count 0 */
ITEM_REGISTER(test_vector_aead_gcm_simple_data,
	      test_vector_aead_t test_vector_aes_aead_gcm_256_decrypt15) = {
	.mode = MBEDTLS_MODE_GCM,
	.id = MBEDTLS_CIPHER_ID_AES,
	.expected_err_code = MBEDTLS_ERR_CIPHER_AUTH_FAILED,
	.crypt_expected_result = EXPECTED_TO_FAIL,
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name = TV_NAME("GCM 256 Decrypt message_len=16 ad_len=90 "
				      "mac_len=13 nonce_len=128 invalid 1"),
	.p_plaintext = "3fe7811a8224a1881da34a27e03da86a",
	.p_ciphertext = "2975341596f99a22f85a48272d089357",
	.p_key =
		"f65818c25506e571ea4778e71b838ab24d3d6a318670885ded4761c2214ae08c",
	.p_ad = "f16c6a6a94a09f7936c718ca182f0e2d8b90de8edecec7257354a02539bee9d232"
		"c04b25d6fcc081e8852d834b7044cfec8b0073c62fc676b6d062693b99e791ddc6"
		"292bee1f5dc39acc18b06bf5c73a64772195b89659b87275",
	.p_nonce =
		"f3d6c665c371db5c8d69ab46ac53eabfd4481a337d005bd0204f5838d770a1bb986808"
		"2542b43732d371c7786ab5e3fa217176f959ede631e373488c996c03c00496ff468cc9"
		"a2a15700e3aef82ae01f598f703e55da6d6cc9cace3c1f2adf6973af9f7f19dd903d7d"
		"0670bc082ec0e97c244426910b6c8e85358eaea8a9807b",
	.p_mac = "f260536b28c1220940044c3593"
};

/* AES GCM - NIST CAVS 14.0 Decrypt with keysize 256 - Count 1 */
ITEM_REGISTER(test_vector_aead_gcm_simple_data,
	      test_vector_aead_t test_vector_aes_aead_gcm_256_decrypt16) = {
	.mode = MBEDTLS_MODE_GCM,
	.id = MBEDTLS_CIPHER_ID_AES,
	.expected_err_code = 0,
	.crypt_expected_result = EXPECTED_TO_PASS,
	.mac_expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name = TV_NAME("GCM 256 Decrypt message_len=16 ad_len=90 "
				      "mac_len=13 nonce_len=128 valid 1"),
	.p_plaintext = "32f7cc94968659f34f23bda8117f065e",
	.p_ciphertext = "8700601301096fbfe50b413a8059202d",
	.p_key =
		"fb9cf2d324f5ca351b37d960f314d602d33c01b21be3fcbe0e5a3c55eb9f7d74",
	.p_ad = "b0b6f49d881e0af5d879219d7acdd8efd7c2561ee5516de0cc32b61d1c8abd9629"
		"bfed1bfdd3cb73e3b39d480af6ea7f9c823f55512a8013ac92b6f3b13efe707dd0"
		"8c4349e6e15bb2fd6ea4cd6de69b8f1b1c290353ea6ec548",
	.p_nonce =
		"53571073c7deffe06b42e3a5cd0d0574ff9ba8afb2fa504420d5fbb1fc6c6aec70b412"
		"d40e4e0e0c0abccda8830d3aa6dcb14514f1648b13920a1cf0bc0dfc7ef26d9304f8c1"
		"a2858c5ae18993120508ead1f6aa1f7f5ed3f470b203045e9d3d97b493c7d6991061d6"
		"2555c90bdbd46fa5fe40a4e762361c951f05ee3ce4dd1a",
	.p_mac = "247b1c2705c6300785ff514d58"
};

/* AES GCM - NIST CAVS 14.0 Decrypt with keysize 256 - Count 2 */
ITEM_REGISTER(test_vector_aead_gcm_simple_data,
	      test_vector_aead_t test_vector_aes_aead_gcm_256_decrypt17) = {
	.mode = MBEDTLS_MODE_GCM,
	.id = MBEDTLS_CIPHER_ID_AES,
	.expected_err_code = MBEDTLS_ERR_CIPHER_AUTH_FAILED,
	.crypt_expected_result = EXPECTED_TO_FAIL,
	.mac_expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name = TV_NAME("GCM 256 Decrypt message_len=16 ad_len=90 "
				      "mac_len=13 nonce_len=128 invalid 2"),
	.p_plaintext = "32f7cc94968659f34f23bda8117f065e",
	.p_ciphertext = "5bfb289d9832cc5dffce4d1d33357951",
	.p_key =
		"b21ef6860b889efdc04ee1cbae0e84a4f31ac9369b01caa901e873ee6f970839",
	.p_ad = "d721291424b17f9ca4f059f208dd7908cdcfd50681641c8dfca185c89e7f1ecf17"
		"61bc32b492d8e5ce9cd873cd18a778595fd9b53878634b285f5383a79e01abf654"
		"2abffbf4f67347193740f73c3dbac654398027315280e2d5",
	.p_nonce =
		"7657f649159a91a7f953e323c236a62f678dab54dd5ae8381419fbcb0ce3d3ec358d46"
		"fba5e4dc67cf4157bc6a8f42bc4b1d8624c0b9501f00146e628cecc6580aff6d1bf59f"
		"5667f3005b2636f4333930b07f8e814966fce1740919d1f3befa418a81693c0be066b1"
		"d17ede09ef36b35b1d908608aeb7ea77d03eec9936736b",
	.p_mac = "8eac04b744d91e7b2c5a6ed792"
};
#endif
#endif

#if defined(MBEDTLS_AES_C)
#if defined(MBEDTLS_CIPHER_MODE_CTR)
const char ctr_long_plain[] = {
	"d8571a7c14be149a0e94fc6c0d8ec2fa0d55510787762e41726d33f96d45f909194fe52571"
	"b7dd556a6016f8063cf1bd1601b4cac12814adf097d20c01ebc74e6ff786895ac85aca48cf"
	"982eb089eed94d0c3f1f33156a01fa7675154971756fa63493cc0d587ff3d2895c782618a6"
	"7f8f7003b7c7fee18e609cc159ad99bc70bc16fda7e01f8352d9a628c861cd97b82b7ebd83"
	"7506a5a14a94e8e7db0589cb5ef10c3808977accc1f261d2e87a5e4556a626a388b83349f3"
	"75b79a35297c294a0deb0dff4c414235a4c3d799a602eb3633d655725e084421c20e5415a1"
	"f11765514d1d8d8800617e3c26cbbe71cc423305f62c4c770bffec44"
};
const char ctr_128_long_cipher[] = {
	"34dbc50f8cde682afc46ea19e710631e3b7e2d3be0057f226acd442e91158aa77363265d09"
	"3eea1ad4d4dee311869df9fe9d8d5531d98c6b249de3d714876cb0dffac1714e42cbc4b8a7"
	"a8b920c24bdb15957b457ef46bf4e9bca48d34f89c749ded3fb54486540ab7e0f04065e0d4"
	"3df2eacd37803db28775c679f15d1c2bc10b8a4481a0f1cbc54c091edd4b7e6183513cd2f5"
	"f8bf4049562120a9ac4844e6f35141df20baf912999cb557e2e9d1501b8497425a091bdd09"
	"d4483ce1a51c1a4fd775f236ae3fa543535e012fa89a1aa81218cf1d3b23572309afa230d8"
	"e6e1814719fb76c82ed66c4dec3c5568d2911e9b8862c8a679b884bc"
};
const char ctr_128_key[] = { "2b7e151628aed2a6abf7158809cf4f3c" };
const char ctr_counter_1[] = { "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff" };
const char ctr_counter_2[] = { "f0f1f2f3f4f5f6f7f8f9fafbfcfdff00" };
const char ctr_counter_3[] = { "f0f1f2f3f4f5f6f7f8f9fafbfcfdff01" };
const char ctr_counter_4[] = { "f0f1f2f3f4f5f6f7f8f9fafbfcfdff02" };
const char ctr_counter_5[] = { "f0f1f2f3f4f5f6f7f8f9fafbfcfdff03" };
const char ctr_plain_1[] = { "6bc1bee22e409f96e93d7e117393172a" };
const char ctr_plain_2[] = { "ae2d8a571e03ac9c9eb76fac45af8e51" };
const char ctr_plain_3[] = { "30c81c46a35ce411e5fbc1191a0a52ef" };
const char ctr_plain_4[] = { "f69f2445df4f9b17ad2b417be66c3710" };
const char ctr_128_cipher_1[] = { "874d6191b620e3261bef6864990db6ce" };
const char ctr_128_cipher_2[] = { "9806f66b7970fdff8617187bb9fffdff" };
const char ctr_128_cipher_3[] = { "5ae4df3edbd5d35e5b4f09020db03eab" };
const char ctr_128_cipher_4[] = { "1e031dda2fbe03d1792170a0f3009cee" };

/* AES CTR - Functional test using test vector NIST SP 800-38A CTR-AES128.Encrypt - Block 1 */
ITEM_REGISTER(test_vector_aes_ctr_func_data,
	      test_vector_aes_t test_vector_aes_ctr_128_functional) = {
	.mode = MBEDTLS_MODE_CTR,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("CTR 128 Functional"),
	.p_plaintext = ctr_plain_1,
	.p_ciphertext = ctr_128_cipher_1,
	.p_key = ctr_128_key,
	.p_iv = ctr_counter_1
};

/* AES CTR - Custom test vector - long */
ITEM_REGISTER(test_vector_aes_ctr_data,
	      test_vector_aes_t test_vector_aes_ctr_128_encrypt_long) = {
	.mode = MBEDTLS_MODE_CTR,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CTR 128 Encrypt message_len=250"),
	.p_plaintext = ctr_long_plain,
	.p_ciphertext = ctr_128_long_cipher,
	.p_key = ctr_128_key,
	.p_iv = ctr_counter_1
};

/* AES CTR - Custom test vector - short */
ITEM_REGISTER(test_vector_aes_ctr_data,
	      test_vector_aes_t test_vector_aes_ctr_128_encrypt_short) = {
	.mode = MBEDTLS_MODE_CTR,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CTR 128 Encrypt message_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key = ctr_128_key,
	.p_iv = ctr_counter_1
};
#endif
#endif

#if defined(MBEDTLS_AES_C)
#if defined(MBEDTLS_CIPHER_MODE_CBC)
/* AES CBC - Functional test using test vector NIST CAVS 11.1 CBC KeySbox 128 - Count 0 */
ITEM_REGISTER(test_vector_aes_cbc_func_data,
	      test_vector_aes_t test_vector_aes_cbc_128_functional) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC 128 Functional"),
	.p_plaintext = "00000000000000000000000000000000",
	.p_ciphertext = "6d251e6944b051e04eaa6fb4dbf78465",
	.p_key = "10a58869d74be5a374cf867cfb473859",
	.p_iv = "00000000000000000000000000000000"
};

/* AES CBC - Custom test vector - Encrypt - Message length 0 */
ITEM_REGISTER(test_vector_aes_cbc_data,
	      test_vector_aes_t test_vector_aes_cbc_128_encrypt_c0) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC 128 Encrypt message_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key = "89df4c819f49dbcbcb124304023cf38c",
	.p_iv = "97ead25a84abd4a55268d1d347faee05"
};

/* AES CBC - Custom test vector - Decrypt - Message length 0 */
ITEM_REGISTER(test_vector_aes_cbc_data,
	      test_vector_aes_t test_vector_aes_cbc_128_decrypt_c0) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name = TV_NAME("CBC 128 Decrypt message_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key = "89df4c819f49dbcbcb124304023cf38c",
	.p_iv = "97ead25a84abd4a55268d1d347faee05"
};
#endif
#endif

#if defined(MBEDTLS_AES_C)
/* AES ECB - Functional test using test vector NIST CAVS 11.1 ECB KeySbox 128 - Count 0 */
ITEM_REGISTER(test_vector_aes_ecb_func_data,
	      test_vector_aes_t test_vector_aes_ecb_128_functional) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("ECB 128 Functional"),
	.p_plaintext = "00000000000000000000000000000000",
	.p_ciphertext = "6d251e6944b051e04eaa6fb4dbf78465",
	.p_key = "10a58869d74be5a374cf867cfb473859"
};

/* AES ECB - Custom test vector - Encrypt - Message length 0 */
ITEM_REGISTER(test_vector_aes_ecb_data,
	      test_vector_aes_t test_vector_aes_ecb_128_encrypt_c0) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("ECB 128 Encrypt message_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key = "89df4c819f49dbcbcb124304023cf38c"
};

/* AES ECB - Custom test vector - Decrypt - Message length 0 */
ITEM_REGISTER(test_vector_aes_ecb_data,
	      test_vector_aes_t test_vector_aes_ecb_128_decrypt_c0) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_DECRYPT,
	.p_test_vector_name = TV_NAME("ECB 128 Decrypt message_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key = "89df4c819f49dbcbcb124304023cf38c"
};
#endif

#if defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED)
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
#endif

#if defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED)
/* ECDSA random - NIST P-256, SHA-256 - first test case */
ITEM_REGISTER(test_vector_ecdsa_random_data,
	      test_vector_ecdsa_random_t
		      test_vector_ecdsa_random_secp256r1_SHA256_1) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.p_test_vector_name = TV_NAME("secp256r1 random SHA256 1"),
	.p_input =
		"44acf6b7e36c1342c2c5897204fe09504e1e2efb1a900377dbc4e7a6a133ec56",
};
#endif /* MBEDTLS_ECP_DP_SECP256R1_ENABLED */

#if defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED)
/* ECDSA sign - NIST CAVS 11.2 P-256, SHA-256 - first test case */
ITEM_REGISTER(
	test_vector_ecdsa_sign_data,
	test_vector_ecdsa_sign_t test_vector_ecdsa_sign_secp256r1_SHA256_1) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_sign_err_code = 0,
	.expected_verify_err_code = 0,
	.p_test_vector_name = TV_NAME("secp256r1 valid SHA256 1"),
	.p_input =
		"44acf6b7e36c1342c2c5897204fe09504e1e2efb1a900377dbc4e7a6a133ec56",
	.p_qx = "1ccbe91c075fc7f4f033bfa248db8fccd3565de94bbfb12f3c59ff46c271bf83",
	.p_qy = "ce4014c68811f9a21a1fdb2c0e6113e06db7ca93b7404e78dc7ccd5ca89a4ca9",
	.p_x = "519b423d715f8b581f4fa8ee59f4771a5b44c8130b4e3eacca54a56dda72b464"
};

/* ECDSA sign - NIST CAVS 11.2 P-256, SHA-256 - second test case */
ITEM_REGISTER(
	test_vector_ecdsa_sign_data,
	test_vector_ecdsa_sign_t test_vector_ecdsa_sign_secp256r1_SHA256_2) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_sign_err_code = 0,
	.expected_verify_err_code = 0,
	.p_test_vector_name = TV_NAME("secp256r1 valid SHA256 2"),
	.p_input =
		"9b2db89cb0e8fa3cc7608b4d6cc1dec0114e0b9ff4080bea12b134f489ab2bbc",
	.p_qx = "e266ddfdc12668db30d4ca3e8f7749432c416044f2d2b8c10bf3d4012aeffa8a",
	.p_qy = "bfa86404a2e9ffe67d47c587ef7a97a7f456b863b4d02cfc6928973ab5b1cb39",
	.p_x = "0f56db78ca460b055c500064824bed999a25aaf48ebb519ac201537b85479813"
};

/* ECDSA sign - NIST CAVS 11.2 P-256, SHA-256 - third test case */
ITEM_REGISTER(
	test_vector_ecdsa_sign_data,
	test_vector_ecdsa_sign_t test_vector_ecdsa_sign_secp256r1_SHA256_3) = {
	.curve_type = MBEDTLS_ECP_DP_SECP256R1,
	.expected_sign_err_code = 0,
	.expected_verify_err_code = 0,
	.p_test_vector_name = TV_NAME("secp256r1 valid SHA256 3"),
	.p_input =
		"b804cf88af0c2eff8bbbfb3660ebb3294138e9d3ebd458884e19818061dacff0",
	.p_qx = "74ccd8a62fba0e667c50929a53f78c21b8ff0c3c737b0b40b1750b2302b0bde8",
	.p_qy = "29074e21f3a0ef88b9efdf10d06aa4c295cc1671f758ca0e4cd108803d0f2614",
	.p_x = "e283871239837e13b95f789e6e1af63bf61c918c992e62bca040d64cad1fc2ef"
};
#endif

#if defined(MBEDTLS_ECP_DP_SECP192R1_ENABLED)
#if defined(MBEDTLS_SHA256_C)
/* ECDSA verify - NIST CAVS 11.0 P-192, SHA-256 */
ITEM_REGISTER(test_vector_ecdsa_verify_data,
	      test_vector_ecdsa_verify_t
		      test_vector_ecdsa_verify_secp192r1_SHA256_1) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = 0,
	.p_test_vector_name = TV_NAME("secp192r1 valid SHA256 1"),
	.p_input =
		"f1d42d1c663fa4d88325458d31fb08b35e8fac7cebc04b224db57439680c9be4",
	.p_qx = "b870597b4b8dc8fc07ed59b6f079e87936d56d0326c17249",
	.p_qy = "e54c404920cd530f0680d8aa2a4fb70b5f8605e6ebbf2751",
	.p_r = "b53dc1abd4f65d5e0506fa146bee65ecb6cd5353830b67ea",
	.p_s = "aa44232f2fa6613f85fda824ded69e4137cdf5688c6b3ba9"
};

/* ECDSA verify - NIST CAVS 11.0 P-192, SHA-256 */
ITEM_REGISTER(test_vector_ecdsa_verify_data,
	      test_vector_ecdsa_verify_t
		      test_vector_ecdsa_verify_secp192r1_SHA256_2) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = 0,
	.p_test_vector_name = TV_NAME("secp192r1 valid SHA256 2"),
	.p_input =
		"cab19f4afca519c6c8a2a09ba7e631ff56cc898694b64123b62e3c94b9fb4696",
	.p_qx = "795bbf28b86af380c2b080e622f92f81de6d2af41a39bc39",
	.p_qy = "3d3bcfcbe704426e95d0edbf40eae25a259af239b00158c9",
	.p_r = "5a3fd911aac408cce41e0eaf42761cce155c5a6efe03df11",
	.p_s = "605ffbb146bf787888d9c3e45f79d0bc6959dcfacfaea437"
};

/* ECDSA verify - NIST CAVS 11.0 P-192, SHA-256 */
ITEM_REGISTER(test_vector_ecdsa_verify_data,
	      test_vector_ecdsa_verify_t
		      test_vector_ecdsa_verify_secp192r1_SHA256_3) = {
	.curve_type = MBEDTLS_ECP_DP_SECP192R1,
	.expected_err_code = 0,
	.p_test_vector_name = TV_NAME("secp192r1 valid SHA256 3"),
	.p_input =
		"786f3a4c00a899bfcd2a79e59ad387562c49e01370ee2fc9feab605a3552e37d",
	.p_qx = "8109731205bd9e363c0521cddf94af58129af3f38d276f2a",
	.p_qy = "9fcf7695165bafb39c2d53b61c4ccfed3891abc6db1fc22c",
	.p_r = "cac3fe60f567724f7afb825aeda68c3b345b44ef3879dc70",
	.p_s = "4544b7d4457b61b66cabfd6174f2c5a594b2c0f300b0e8ea"
};
#endif
#endif

#if defined(MBEDTLS_ECJPAKE_C)
/*
 * Test data as used by ARMmbed: https://github.com/ARMmbed/mbed-crypto/blob/master/library/ecjpake.c
 */

static const unsigned char ecjpake_password[] =
	"7468726561646a70616b6574657374";
static const unsigned char ecjpake_x1[] =
	"0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f21";
static const unsigned char ecjpake_x2[] =
	"6162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f81";
static const unsigned char ecjpake_x3[] =
	"6162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f81";
static const unsigned char ecjpake_x4[] =
	"c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe1";
static const unsigned char ecjpake_round_msg_cli_1[] =
	"4104accf0106ef858fa2d919331346805a78b58bbad0b844e5c7892879146187dd2666ada7"
	"81bb7f111372251a8910621f634df128ac48e381fd6ef9060731f694a441041dd0bd5d4566"
	"c9bed9ce7de701b5e82e08e84b730466018ab903c79eb982172236c0c1728ae4bf73610d34"
	"de44246ef3d9c05a2236fb66a6583d7449308babce2072fe16662992e9235c25002f11b150"
	"87b82738e03c945bf7a2995dda1e98345841047ea6e3a4487037a9e0dbd79262b2cc273e77"
	"9930fc18409ac5361c5fe669d702e147790aeb4ce7fd6575ab0f6c7fd1c335939aa863ba37"
	"ec91b7e32bb013bb2b4104a49558d32ed1ebfc1816af4ff09b55fcb4ca47b2a02d1e7caf11"
	"79ea3fe1395b22b861964016fabaf72c975695d93d4df0e5197fe9f040634ed59764937787"
	"be20bc4deebbf9b8d60a335f046ca3aa941e45864c7cadef9cf75b3d8b010e443ef0";
static const unsigned char ecjpake_round_msg_cli_2[] =
	"410469d54ee85e90ce3f1246742de507e939e81d1dc1c5cb988b58c310c9fdd9524d93720b"
	"45541c83ee8841191da7ced86e3312d43623c1d63e74989aba4affd1ee4104077e8c31e20e"
	"6bedb760c13593e69f15be85c27d68cd09ccb8c4183608917c5c3d409fac39fefee82f7292"
	"d36f0d23e055913f45a52b85dd8a2052e9e129bb4d200f011f19483535a6e89a580c9b0003"
	"baf21462ece91a82cc38dbdcae60d9c54c";
static const unsigned char ecjpake_round_msg_srv_1[] =
	"41047ea6e3a4487037a9e0dbd79262b2cc273e779930fc18409ac5361c5fe669d702e14779"
	"0aeb4ce7fd6575ab0f6c7fd1c335939aa863ba37ec91b7e32bb013bb2b410409f85b3d20eb"
	"d7885ce464c08d056d6428fe4dd9287aa365f131f4360ff386d846898bc4b41583c2a5197f"
	"65d78742746c12a5ec0a4ffe2f270a750a1d8fb51620934d74eb43e54df424fd96306c0117"
	"bf131afabf90a9d33d1198d905193735144104190a07700ffa4be6ae1d79ee0f06aeb544cd"
	"5addaabedf70f8623321332c54f355f0fbfec783ed359e5d0bf7377a0fc4ea7ace473c9c11"
	"2b41ccd41ac56a56124104360a1cea33fce641156458e0a4eac219e96831e6aebc88b3f375"
	"2f93a0281d1bf1fb106051db9694a8d6e862a5ef1324a3d9e27894f1ee4f7c59199965a8dd"
	"4a2091847d2d22df3ee55faa2a3fb33fd2d1e055a07a7c61ecfb8d80ec00c2c9eb12";
static const unsigned char ecjpake_round_msg_srv_2[] =
	"03001741040fb22b1d5d1123e0ef9feb9d8a2e590a1f4d7ced2c2b06586e8f2a16d4eb2fda"
	"4328a20b07d8fd667654ca18c54e32a333a0845451e926ee8804fd7af0aaa7a641045516ea"
	"3e54a0d5d8b2ce786b38d383370029a5dbe4459c9dd601b408a24ae6465c8ac905b9eb03b5"
	"d3691c139ef83f1cd4200f6c9cd4ec392218a59ed243d3c820ff724a9a70b88cb86f20b434"
	"c6865aa1cd7906dd7c9bce3525f508276f26836c";
static const unsigned char ecjpake_ss[] =
	"f3d47f599844db92a569bbe7981e39d931fd743bf22e98f9b438f719d3c4f351";

/*
 * Uses empty initial data on both sides and deterministic rng.
 * Derive a secret for both client and server.
 * Should verify:
 *      Derived secrets same length.
 *      Derived secrets equal data.
 */
ITEM_REGISTER(
	test_vector_ecjpake_random_data,
	test_vector_ecjpake_t test_vector_ecjpake_trivial_random_handshake) = {
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_password = ecjpake_password,
	.p_test_vector_name = TV_NAME("Trivial random handshake")
};

/*
 * Uses pre-made private keys to generate public keys.
 * Thus only ECJPAKE reads are done, not writes.
 * Messages are also pre-defined.
 * Should verify:
 *      Derived secret client same length as pre-made secret.
 *      Derived secret server same length as pre-made secret.
 *      Derived secret client equal data in pre-made secret.
 *      Derived secret server equal data in pre-made secret.
 */
ITEM_REGISTER(test_vector_ecjpake_given_data,
	      test_vector_ecjpake_t test_vector_ecjpake_given_data_001) = {
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("Predefined private keys"),
	.p_password = ecjpake_password,
	.p_priv_key_client_1 = ecjpake_x1,
	.p_priv_key_client_2 = ecjpake_x2,
	.p_priv_key_server_1 = ecjpake_x3,
	.p_priv_key_server_2 = ecjpake_x4,
	.p_round_message_client_1 = ecjpake_round_msg_cli_1,
	.p_round_message_client_2 = ecjpake_round_msg_cli_2,
	.p_round_message_server_1 = ecjpake_round_msg_srv_1,
	.p_round_message_server_2 = ecjpake_round_msg_srv_2,
	.p_expected_shared_secret = ecjpake_ss,
};
#endif /* MBEDTLS_ECJPAKE_C */

#if defined(MBEDTLS_SHA256_C)
const char hkdf_ikm_len_22[] = {
	"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b"
};
const char hkdf_ikm_len_80[] = {
	"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021222324"
	"25262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40414243444546474849"
	"4a4b4c4d4e4f"
};
const char hkdf_ikm_len_250[] = {
	"d9ffac12ae7a19e13c9e92b21e341bff5e2a949f240a55595d9cbcd77512480b435237341e"
	"dbc831dfc524f35ad8b95d238629d66849d5b66edda67907cbe5556f175a3dedd8f5e5d5ef"
	"12102fa1dde3e279b559130d0b441f1a20c04f5dbeb2bf0912272f29b96c390fba1b36a951"
	"fea808275c8713b9685398bbbb1ba64f069b231f49c3095d2c95471b27df56acd671d7cbe7"
	"817826d107815af721f3f7d262c651f1ebae961979778eb37dac8ce75f1efdb703789764a0"
	"d34600ffc056e331dab60b1d207a5935649fb75e5a8d3ea6b09a20954736fad51a4b031a2e"
	"961efa85b65a7c7b02345c199e90d8be40bb28496ad1ea93c9daeb55"
};
const char hkdf_dummy_okm[] = { "3a8d5dc16eba7ac69b38" };
const char hkdf_salt_len_13[] = { "000102030405060708090a0b0c" };
const char hkdf_salt_len_20[] = { "1911bff47c578781d0609cb563bb7da69c27fd18" };
const char hkdf_salt_len_80[] = {
	"606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f8081828384"
	"85868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9"
	"aaabacadaeaf"
};
const char hkdf_salt_len_250[] = {
	"7d1ca5017a99229aae0e03c5bfb98b1425e8649956123341543edd6e75600a73fad29a00e7"
	"06d68643133758b3602e4522071704eb212dfd613ab48f8826a90c944f626c13f6762d4361"
	"a322d4f322c52d659ec0dbabf70d9e2daca42bb2333f5d9bbea1376a2fd895c5dda48270c1"
	"757100e9083973e2fb0cdbbeadd44e40309887f300ca59a71167347f5d2e5583b4d5525ded"
	"f7f57692397828c7009a834df2f2a711e9c892f6c706d3bf10a8c07db4c42ab74d75f8e249"
	"e879a5e96329f04600e1bf81ee567ce0c06bd2a1f969feca465a75f996e7a79523b89ef8e2"
	"2b85ecf436d7cfe7b4e6f535e0a661398ca5711f3f4513d864381702"
};
const char hkdf_info_len_10[] = { "f0f1f2f3f4f5f6f7f8f9" };
const char hkdf_info_len_20[] = { "3a594a18b699ef8819008ed38c3aa4320581db9d" };
const char hkdf_info_len_80[] = {
	"b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4"
	"d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9"
	"fafbfcfdfeff"
};
const char hkdf_info_len_250[] = {
	"ccfafb6a30475d53b3a2d420825c54d7e026874e913173108ac83c99aae278b1850538123e"
	"c8f9dd00cc18cf0a66d271bf69c8035f1301c11a241eaaf9be56b99b65ef1a596fdb49e46a"
	"acbfb39cd0afcd45516ab3b2f7e9fbf16246642f24ffc1d04d5d5e5694569d207feef75ba9"
	"55cb119fa4f691a9bce51d32281795ffbd41fb157387b911e252a676b9dbf8e94e0e371495"
	"b15602b527ebcfe1ae1773196fad39de7c5351869724f7ef17586ae9d85af304ba132db50f"
	"a1449e297f650325558824291b88cbac38f7d27a24d8c18f0ba6bd0e749079184693e21ffe"
	"9ee3dae068485e8b22663696f2d70a04e61ceceeb2da839bfbb30838"
};

/* HKDF - SHA-256 Custom test case 1 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_256_c1) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_err_code_expand = MBEDTLS_ERR_HKDF_BAD_INPUT_DATA,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("SHA256 Expand and Extract ikm_len=0 "
				      "okm_len=10 salt_len=13 info_len=10"),
	.p_ikm = "",
	.p_okm = hkdf_dummy_okm,
	.p_prk = "",
	.p_salt = hkdf_salt_len_20,
	.p_info = hkdf_info_len_20
};

/* HKDF - SHA-256 Custom test case 2 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_256_c2) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME(
		"SHA256 Expand and Extract ikm_len=1 okm_len=1 salt_len=1 info_len=1"),
	.p_ikm = "ab",
	.p_okm = "53",
	.p_prk =
		"1f3624af63d5221a80b6d6cbb7d372e595cb512f4ad248643d8d0a74f0be8335",
	.p_salt = "1b",
	.p_info = "6f"
};

/* HKDF - SHA-256 Custom test case 3 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_256_c3) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME(
		"SHA256 Expand and Extract Invalid ikm_len=20 okm_len=0 "
		"salt_len=13 info_len=10"),
	.p_ikm = hkdf_ikm_len_22,
	.p_okm = "",
	.p_prk =
		"077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5",
	.p_salt = hkdf_salt_len_13,
	.p_info = hkdf_info_len_10
};
#endif

#if defined(MBEDTLS_SHA256_C)
/* HMAC - NIST CAVS 11.0 */
ITEM_REGISTER(test_vector_hmac_data,
	      test_vector_hmac_t test_vector_hmac256_inv_message) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("SHA256 invalid - message changed"),
	.p_input =
		"c1689c2591eaf3c9e66070f8a77954ffb81749f1b00346f9dfe0b2ee905dcc288baf4a"
		"92de3f4001dd9f44c468c3d07d6c6ee82faceafc97c2fc0fc0601719d2dcd0aa2aec92"
		"d1b0ae933c65eb06a03c9c935c2bad0459810241347ab87e9f11adb30415424c6c7f5f"
		"22a003b8ab8de54f6ded0e3ab9245fa79568451dfa258e",
	.p_key = "9779d9120642797f1747025d5b22b7ac607cab08e1758f2f3a46c8be1e25c53b8"
		 "c6a8f58ffefa176",
	.p_expected_output =
		"769f00d3e6a6cc1fb426a14a4f76c6462e6149726e0dee0ec0cf97a16605ac8b"
};

/* HMAC - NIST CAVS 11.0 */
ITEM_REGISTER(test_vector_hmac_data,
	      test_vector_hmac_t test_vector_hmac256_inv_key) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("SHA256 invalid - key changed"),
	.p_input =
		"b1689c2591eaf3c9e66070f8a77954ffb81749f1b00346f9dfe0b2ee905dcc288baf4a"
		"92de3f4001dd9f44c468c3d07d6c6ee82faceafc97c2fc0fc0601719d2dcd0aa2aec92"
		"d1b0ae933c65eb06a03c9c935c2bad0459810241347ab87e9f11adb30415424c6c7f5f"
		"22a003b8ab8de54f6ded0e3ab9245fa79568451dfa258e",
	.p_key = "a779d9120642797f1747025d5b22b7ac607cab08e1758f2f3a46c8be1e25c53b8"
		 "c6a8f58ffefa176",
	.p_expected_output =
		"769f00d3e6a6cc1fb426a14a4f76c6462e6149726e0dee0ec0cf97a16605ac8b"
};

/* HMAC - NIST CAVS 11.0 */
ITEM_REGISTER(test_vector_hmac_data,
	      test_vector_hmac_t test_vector_hmac256_inv_sign) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("SHA256 invalid - signature changed"),
	.p_input =
		"b1689c2591eaf3c9e66070f8a77954ffb81749f1b00346f9dfe0b2ee905dcc288baf4a"
		"92de3f4001dd9f44c468c3d07d6c6ee82faceafc97c2fc0fc0601719d2dcd0aa2aec92"
		"d1b0ae933c65eb06a03c9c935c2bad0459810241347ab87e9f11adb30415424c6c7f5f"
		"22a003b8ab8de54f6ded0e3ab9245fa79568451dfa258e",
	.p_key = "9779d9120642797f1747025d5b22b7ac607cab08e1758f2f3a46c8be1e25c53b8"
		 "c6a8f58ffefa176",
	.p_expected_output =
		"869f00d3e6a6cc1fb426a14a4f76c6462e6149726e0dee0ec0cf97a16605ac8b"
};
#endif

#if defined(MBEDTLS_SHA256_C)
const char flash_data_sha_256[4096] = { "1234567890" };

/* SHA256 - Based on NIST CAVS 11.0 */
ITEM_REGISTER(test_vector_hash_256_data,
	      test_vector_hash_t test_vector_SHA256_invalid) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("SHA256 invalid hash"),
	.p_input =
		"6a86b737eaea8ee976a0a24da63e7ed7eefad18a101c1211e2b3650c5187c2a8a65054"
		"7208251f6d4237e661c7bf4c77f335390394c37fa1a9f9be836ac28509",
	.p_expected_output =
		"42e61e174fbb3897d6dd6cef3dd2802fe67b331953b06114a65c772859dfc1aa"
};

/* SHA256 - NIST CAVS 11.0 */
ITEM_REGISTER(test_vector_hash_256_data,
	      test_vector_hash_t test_vector_SHA256_0) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA256 message_len=0"),
	.p_input = "",
	.p_expected_output =
		"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
};

/* SHA256 - NIST CAVS 11.0 */
ITEM_REGISTER(test_vector_hash_256_data,
	      test_vector_hash_t test_vector_SHA256_4) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA256 message_len=4"),
	.p_input = "c98c8e55",
	.p_expected_output =
		"7abc22c0ae5af26ce93dbb94433a0e0b2e119d014f8e7f65bd56c61ccccd9504"
};
#endif

#if defined(MBEDTLS_SHA512_C)
const char flash_data_sha_512[4096] = { "1234567890" };

/* SHA512 - Based on NIST CAVS 11.0 */
ITEM_REGISTER(test_vector_hash_512_data,
	      test_vector_hash_t test_vector_sha512_invalid) = {
	.digest_type = MBEDTLS_MD_SHA512,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("SHA512 invalid hash"),
	.p_input =
		"d1ca70ae1279ba0b918157558b4920d6b7fba8a06be515170f202fafd36fb7f79d69fa"
		"d745dba6150568db1e2b728504113eeac34f527fc82f2200b462ecbf5d",
	.p_expected_output =
		"046e46623912b3932b8d662ab42583423843206301b58bf20ab6d76fd47f1cbbcf421d"
		"f536ecd7e56db5354e7e0f98822d2129c197f6f0f222b8ec5231f3967d"
};

/* SHA512 - NIST CAVS 11.0 */
ITEM_REGISTER(test_vector_hash_512_data,
	      test_vector_hash_t test_vector_sha512_0) = {
	.digest_type = MBEDTLS_MD_SHA512,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA512 message_len=0"),
	.p_input = "",
	.p_expected_output =
		"cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d1"
		"3c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"
};

/* SHA512 - NIST CAVS 11.0 */
ITEM_REGISTER(test_vector_hash_512_data,
	      test_vector_hash_t test_vector_sha512_4) = {
	.digest_type = MBEDTLS_MD_SHA512,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA512 message_len=4"),
	.p_input = "a801e94b",
	.p_expected_output =
		"dadb1b5a27f9fece8d86adb2a51879beb1787ff28f4e8ce162cad7fee0f942efcabbf7"
		"38bc6f797fc7cc79a3a75048cd4c82ca0757a324695bfb19a557e56e2f"
};
#endif
