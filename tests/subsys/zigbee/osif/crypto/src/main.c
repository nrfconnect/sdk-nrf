/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zb_nrf_crypto.h>
#include <zboss_api.h>

LOG_MODULE_REGISTER(zboss_osif, CONFIG_ZBOSS_OSIF_LOG_LEVEL);

#define AES_KEY_LENGTH       16
#define AES_PLAINTEXT_LENGTH 16

/* AES test values (taken from FIPS-197) */
uint8_t aes_key[AES_KEY_LENGTH] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
uint8_t aes_plaintext[AES_PLAINTEXT_LENGTH] = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};
uint8_t aes_ciphertext[AES_PLAINTEXT_LENGTH] = {
	0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
	0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a
};


ZTEST_SUITE(nrf_osif_crypto_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(nrf_osif_crypto_tests, test_crypto)
{
	uint8_t aes_encrypted[AES_PLAINTEXT_LENGTH] = {};
	int i;

	zb_osif_aes_init();
	zb_osif_aes128_hw_encrypt(aes_key, aes_plaintext, aes_encrypted);

	for (i = 0; i < AES_PLAINTEXT_LENGTH; i++) {
		zassert_equal(aes_encrypted[i], aes_ciphertext[i],
			      "Encrypted data mismatch at byte %d", i);
	}
}
