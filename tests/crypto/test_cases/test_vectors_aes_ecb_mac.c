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

/**@brief CMAC test vectors can be found on NIST web pages.
 *
 * https://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/CAVP-TESTING-BLOCK-CIPHER-MODES#CMAC
 */

/* AES CMAC - NIST SP 800-38B, CMAC-AES128, Example #1 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_128_1) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 128 message_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key = "2b7e151628aed2a6abf7158809cf4f3c"
};

/* AES CMAC - NIST SP 800-38B, CMAC-AES128, Example #2 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_128_2) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 128 message_len=16"),
	.p_plaintext = "6bc1bee22e409f96e93d7e117393172a",
	.p_ciphertext = "070a16b46b4d4144f79bdd9dd04a287c",
	.p_key = "2b7e151628aed2a6abf7158809cf4f3c"
};

/* AES CMAC - NIST SP 800-38B, CMAC-AES128, Example #3 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_128_3) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 128 message_len=20"),
	.p_plaintext = "6bc1bee22e409f96e93d7e117393172aae2d8a57",
	.p_ciphertext = "7d85449ea6ea19c823a7bf78837dfade",
	.p_key = "2b7e151628aed2a6abf7158809cf4f3c"
};

/* AES CMAC - NIST SP 800-38B, CMAC-AES128, Example #4 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_128_4) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 128 message_len=64"),
	.p_plaintext =
		"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c"
		"46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
	.p_ciphertext = "51f0bebf7e3b9d92fc49741779363cfe",
	.p_key = "2b7e151628aed2a6abf7158809cf4f3c"
};

#if defined(CONFIG_MBEDTLS_AES_256_CMAC_C)
/* AES CMAC - NIST SP 800-38B, CMAC-AES192, Example #1 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_192_1) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 192 message_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key = "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b"
};

/* AES CMAC - NIST SP 800-38B, CMAC-AES192, Example #2 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_192_2) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 192 message_len=16"),
	.p_plaintext = "6bc1bee22e409f96e93d7e117393172a",
	.p_ciphertext = "9e99a7bf31e710900662f65e617c5184",
	.p_key = "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b"
};

/* AES CMAC - NIST SP 800-38B, CMAC-AES192, Example #3 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_192_3) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 192 message_len=20"),
	.p_plaintext = "6bc1bee22e409f96e93d7e117393172aae2d8a57",
	.p_ciphertext = "3d75c194ed96070444a9fa7ec740ecf8",
	.p_key = "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b"
};

/* AES CMAC - NIST SP 800-38B, CMAC-AES192, Example #4 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_192_4) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 192 message_len=64"),
	.p_plaintext =
		"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c"
		"46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
	.p_ciphertext = "a1d5df0eed790f794d77589659f39a11",
	.p_key = "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b"
};

/* AES CMAC - NIST SP 800-38B, CMAC-AES256, Example #1 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_256_1) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 256 message_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key = "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"
};

/* AES CMAC - NIST SP 800-38B, CMAC-AES256, Example #2 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_256_2) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 256 message_len=16"),
	.p_plaintext = "6bc1bee22e409f96e93d7e117393172a",
	.p_ciphertext = "28a7023f452e8f82bd4bf28d8c37c35c",
	.p_key = "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"
};

/* AES CMAC - NIST SP 800-38B, CMAC-AES256, Example #3 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_256_3) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 256 message_len=20"),
	.p_plaintext = "6bc1bee22e409f96e93d7e117393172aae2d8a57",
	.p_ciphertext = "156727dc0878944a023c1fe03bad6d93",
	.p_key = "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"
};

/* AES CMAC - NIST SP 800-38B, CMAC-AES256, Example #4 */
ITEM_REGISTER(test_vector_aes_ecb_mac_data,
	      test_vector_aes_t test_vector_aes_cmac_256_4) = {
	.mode = MBEDTLS_MODE_ECB,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CMAC 256 message_len=64"),
	.p_plaintext =
		"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c"
		"46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
	.p_ciphertext = "e1992190549f6ed5696a2c056c315410",
	.p_key = "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"
};

#endif /* MBEDTLS_AES_256_CMAC_C */
