/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <stddef.h>
#include <stdint.h>
#include <soc.h>
#include <ble_controller_soc.h>
#include <random/rand32.h>

#include "nrf_errno.h"
#include "multithreading_lock.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_KEYS)
#define LOG_MODULE_NAME ble_controller_crypto
#include <common/log.h>

#define BT_ECB_BLOCK_SIZE 16

int bt_rand(void *buf, size_t len)
{
	if (buf == NULL) {
		return -NRF_EINVAL;
	}

	u8_t *buf8 = buf;
	size_t bytes_left = len;

	while (bytes_left) {
		u32_t v = sys_rand32_get();

		if (bytes_left >= sizeof(v)) {
			memcpy(buf8, &v, sizeof(v));

			buf8 += sizeof(v);
			bytes_left -= sizeof(v);
		} else {
			memcpy(buf8, &v, bytes_left);
			break;
		}
	}

	return 0;
}

int bt_encrypt_le(const u8_t key[BT_ECB_BLOCK_SIZE],
		  const u8_t plaintext[BT_ECB_BLOCK_SIZE],
		  u8_t enc_data[BT_ECB_BLOCK_SIZE])
{
	u8_t key_le[BT_ECB_BLOCK_SIZE];
	u8_t plaintext_le[BT_ECB_BLOCK_SIZE];
	u8_t enc_data_le[BT_ECB_BLOCK_SIZE];

	BT_HEXDUMP_DBG(key, BT_ECB_BLOCK_SIZE, "key");
	BT_HEXDUMP_DBG(plaintext, BT_ECB_BLOCK_SIZE, "plaintext");

	sys_memcpy_swap(key_le, key, BT_ECB_BLOCK_SIZE);
	sys_memcpy_swap(plaintext_le, plaintext, BT_ECB_BLOCK_SIZE);

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		errcode = ble_controller_ecb_block_encrypt(key_le, plaintext_le,
							   enc_data_le);
		MULTITHREADING_LOCK_RELEASE();
	}

	if (!errcode) {
		sys_memcpy_swap(enc_data, enc_data_le, BT_ECB_BLOCK_SIZE);

		BT_HEXDUMP_DBG(enc_data, BT_ECB_BLOCK_SIZE, "enc_data");
	}

	return errcode;
}

int bt_encrypt_be(const u8_t key[BT_ECB_BLOCK_SIZE],
		  const u8_t plaintext[BT_ECB_BLOCK_SIZE],
		  u8_t enc_data[BT_ECB_BLOCK_SIZE])
{
	BT_HEXDUMP_DBG(key, BT_ECB_BLOCK_SIZE, "key");
	BT_HEXDUMP_DBG(plaintext, BT_ECB_BLOCK_SIZE, "plaintext");

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		errcode = ble_controller_ecb_block_encrypt(key, plaintext,
							   enc_data);
		MULTITHREADING_LOCK_RELEASE();
	}

	if (!errcode) {
		BT_HEXDUMP_DBG(enc_data, BT_ECB_BLOCK_SIZE, "enc_data");
	}

	return errcode;
}
