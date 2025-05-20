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
#include <mbedtls/cipher.h>

/**@brief AES CBC-MAC is tested using only custom generated test vectors.
 *
 */
/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_128_1) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 128 message_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key = "2b7e151628aed2a6abf7158809cf4f3c",
	.p_iv = "00000000000000000000000000000000"
};

/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_128_2) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 128 message_len=16"),
	.p_plaintext = "6bc1bee22e409f96e93d7e117393172a",
	.p_ciphertext = "3ad77bb40d7a3660a89ecaf32466ef97",
	.p_key = "2b7e151628aed2a6abf7158809cf4f3c",
	.p_iv = "00000000000000000000000000000000"
};

/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_128_3) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 128 message_len=32"),
	.p_plaintext =
		"d602b63eebba5b8fe1db84d8ca71abf5023e147508ce206c9732a28cc94eaabc",
	.p_ciphertext = "5a88111a1e75ccc0ebad8b7b74e1c6d1",
	.p_key = "2b7e151628aed2a6abf7158809cf4f3c",
	.p_iv = "00000000000000000000000000000000"
};

/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_128_4) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 128 message_len=64"),
	.p_plaintext =
		"93cccff91971303929c53d0de3dd7a96851e54fe1b484d240cae8ff2b99051766f4d6f"
		"7b500d26e2e43295bd4c6313bca988875944215d8de20298e3bb795d9d",
	.p_ciphertext = "289df89c9703958b37b1b9b1f7842984",
	.p_key = "2b7e151628aed2a6abf7158809cf4f3c",
	.p_iv = "00000000000000000000000000000000"
};

#if defined(MBEDTLS_CIPHER_AES_256_CBC_C)

/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_192_1) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 192 message_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key = "291dc8f04aabad1d63b9820389329e2b4db30bc94264f677",
	.p_iv = "00000000000000000000000000000000"
};

/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_192_2) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 192 message_len=16"),
	.p_plaintext = "6bc1bee22e409f96e93d7e117393172a",
	.p_ciphertext = "9cd89b028e16378d71ac45fdc45b7b08",
	.p_key = "291dc8f04aabad1d63b9820389329e2b4db30bc94264f677",
	.p_iv = "00000000000000000000000000000000"
};

/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_192_3) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 192 message_len=32"),
	.p_plaintext =
		"d602b63eebba5b8fe1db84d8ca71abf5023e147508ce206c9732a28cc94eaabc",
	.p_ciphertext = "098f08d19aef1c6bf63f38c5aa2b9b31",
	.p_key = "291dc8f04aabad1d63b9820389329e2b4db30bc94264f677",
	.p_iv = "00000000000000000000000000000000"
};

/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_192_4) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 192 message_len=64"),
	.p_plaintext =
		"93cccff91971303929c53d0de3dd7a96851e54fe1b484d240cae8ff2b99051766f4d6f"
		"7b500d26e2e43295bd4c6313bca988875944215d8de20298e3bb795d9d",
	.p_ciphertext = "5e0eb2751c73e2b7de96c302caed1459",
	.p_key = "291dc8f04aabad1d63b9820389329e2b4db30bc94264f677",
	.p_iv = "00000000000000000000000000000000"
};

/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_256_1) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_FAIL,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 256 message_len=0"),
	.p_plaintext = "",
	.p_ciphertext = "",
	.p_key =
		"603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
	.p_iv = "00000000000000000000000000000000"
};

/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_256_2) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 256 message_len=16"),
	.p_plaintext = "6bc1bee22e409f96e93d7e117393172a",
	.p_ciphertext = "f3eed1bdb5d2a03c064b5a7e3db181f8",
	.p_key =
		"603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
	.p_iv = "00000000000000000000000000000000"
};

/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_256_3) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 256 message_len=32"),
	.p_plaintext =
		"d602b63eebba5b8fe1db84d8ca71abf5023e147508ce206c9732a28cc94eaabc",
	.p_ciphertext = "b0d33b64ae39d12fdd26cb39657b9047",
	.p_key =
		"603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
	.p_iv = "00000000000000000000000000000000"
};

/* AES CBC MAC - Custom test vector */
ITEM_REGISTER(test_vector_aes_cbc_mac_data,
	      test_vector_aes_t test_vector_aes_cbc_mac_256_4) = {
	.mode = MBEDTLS_MODE_CBC,
	.padding = MBEDTLS_PADDING_NONE,
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.direction = MBEDTLS_ENCRYPT,
	.p_test_vector_name = TV_NAME("CBC MAC 256 message_len=64"),
	.p_plaintext =
		"93cccff91971303929c53d0de3dd7a96851e54fe1b484d240cae8ff2b99051766f4d6f"
		"7b500d26e2e43295bd4c6313bca988875944215d8de20298e3bb795d9d",
	.p_ciphertext = "8dbe503b77ebed416cd0ad049314aaa4",
	.p_key =
		"603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
	.p_iv = "00000000000000000000000000000000"
};
#endif /* MBEDTLS_CIPHER_AES_256_CBC_C */
