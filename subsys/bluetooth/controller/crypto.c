/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <stddef.h>
#include <stdint.h>
#include <soc.h>
#include <mpsl_ecb.h>
#include <zephyr/drivers/entropy.h>

#include "nrf_errno.h"
#include "multithreading_lock.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_sdc_crypto);

#define BT_ECB_BLOCK_SIZE 16

static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));

int bt_rand(void *buf, size_t len)
{
	if (unlikely(!device_is_ready(dev))) {
		return -ENODEV;
	}

	return entropy_get_entropy(dev, (uint8_t *)buf, len);
}

int bt_encrypt_le(const uint8_t key[BT_ECB_BLOCK_SIZE],
		  const uint8_t plaintext[BT_ECB_BLOCK_SIZE],
		  uint8_t enc_data[BT_ECB_BLOCK_SIZE])
{
	LOG_HEXDUMP_DBG(key, BT_ECB_BLOCK_SIZE, "key");
	LOG_HEXDUMP_DBG(plaintext, BT_ECB_BLOCK_SIZE, "plaintext");

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		mpsl_ecb_hal_data_t ecb_data;

		sys_memcpy_swap(ecb_data.key, key, BT_ECB_BLOCK_SIZE);
		sys_memcpy_swap(ecb_data.cleartext, plaintext, BT_ECB_BLOCK_SIZE);
		mpsl_ecb_block_encrypt(&ecb_data);
		sys_memcpy_swap(enc_data, ecb_data.ciphertext, BT_ECB_BLOCK_SIZE);

		MULTITHREADING_LOCK_RELEASE();

		LOG_HEXDUMP_DBG(enc_data, BT_ECB_BLOCK_SIZE, "enc_data");
	}

	return errcode;
}

int bt_encrypt_be(const uint8_t key[BT_ECB_BLOCK_SIZE],
		  const uint8_t plaintext[BT_ECB_BLOCK_SIZE],
		  uint8_t enc_data[BT_ECB_BLOCK_SIZE])
{
	LOG_HEXDUMP_DBG(key, BT_ECB_BLOCK_SIZE, "key");
	LOG_HEXDUMP_DBG(plaintext, BT_ECB_BLOCK_SIZE, "plaintext");

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		mpsl_ecb_hal_data_t ecb_data;

		memcpy(ecb_data.key, key, BT_ECB_BLOCK_SIZE);
		memcpy(ecb_data.cleartext, plaintext, BT_ECB_BLOCK_SIZE);
		mpsl_ecb_block_encrypt(&ecb_data);
		memcpy(enc_data, ecb_data.ciphertext, BT_ECB_BLOCK_SIZE);

		MULTITHREADING_LOCK_RELEASE();

		LOG_HEXDUMP_DBG(enc_data, BT_ECB_BLOCK_SIZE, "enc_data");
	}

	return errcode;
}
