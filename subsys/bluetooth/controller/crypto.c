/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <stddef.h>
#include <stdint.h>
#include <soc.h>
#include <sdc_soc.h>
#include <drivers/entropy.h>

#include "nrf_errno.h"
#include "multithreading_lock.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_KEYS)
#define LOG_MODULE_NAME sdc_crypto
#include <common/log.h>

#define BT_ECB_BLOCK_SIZE 16

int bt_rand(void *buf, size_t len)
{
	static const struct device *dev;

	if (unlikely(!dev)) {
		dev = device_get_binding(DT_LABEL(DT_NODELABEL(rng)));

		if (!dev) {
			return -ENODEV;
		}
	}

	return entropy_get_entropy(dev, (uint8_t *)buf, len);
}

int bt_encrypt_le(const uint8_t key[BT_ECB_BLOCK_SIZE],
		  const uint8_t plaintext[BT_ECB_BLOCK_SIZE],
		  uint8_t enc_data[BT_ECB_BLOCK_SIZE])
{
	uint8_t key_le[BT_ECB_BLOCK_SIZE];
	uint8_t plaintext_le[BT_ECB_BLOCK_SIZE];
	uint8_t enc_data_le[BT_ECB_BLOCK_SIZE];

	BT_HEXDUMP_DBG(key, BT_ECB_BLOCK_SIZE, "key");
	BT_HEXDUMP_DBG(plaintext, BT_ECB_BLOCK_SIZE, "plaintext");

	sys_memcpy_swap(key_le, key, BT_ECB_BLOCK_SIZE);
	sys_memcpy_swap(plaintext_le, plaintext, BT_ECB_BLOCK_SIZE);

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		errcode = sdc_soc_ecb_block_encrypt(key_le, plaintext_le, enc_data_le);
		MULTITHREADING_LOCK_RELEASE();
	}

	if (!errcode) {
		sys_memcpy_swap(enc_data, enc_data_le, BT_ECB_BLOCK_SIZE);

		BT_HEXDUMP_DBG(enc_data, BT_ECB_BLOCK_SIZE, "enc_data");
	}

	return errcode;
}

int bt_encrypt_be(const uint8_t key[BT_ECB_BLOCK_SIZE],
		  const uint8_t plaintext[BT_ECB_BLOCK_SIZE],
		  uint8_t enc_data[BT_ECB_BLOCK_SIZE])
{
	BT_HEXDUMP_DBG(key, BT_ECB_BLOCK_SIZE, "key");
	BT_HEXDUMP_DBG(plaintext, BT_ECB_BLOCK_SIZE, "plaintext");

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		errcode = sdc_soc_ecb_block_encrypt(key, plaintext, enc_data);
		MULTITHREADING_LOCK_RELEASE();
	}

	if (!errcode) {
		BT_HEXDUMP_DBG(enc_data, BT_ECB_BLOCK_SIZE, "enc_data");
	}

	return errcode;
}
