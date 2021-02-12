/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sys/__assert.h>
#include <random/rand32.h>
#include <logging/log.h>
#include <zboss_api.h>
#if CONFIG_CRYPTO_NRF_ECB
#include <crypto/cipher.h>
#elif CONFIG_BT_CTLR
#include <bluetooth/crypto.h>
#elif CONFIG_ZIGBEE_USE_SOFTWARE_AES
#include <tinycrypt/aes.h>
#include <tinycrypt/constants.h>
#else
#error No crypto suite for Zigbee stack has been selected
#endif

#include "zb_nrf_crypto.h"

#define ECB_AES_KEY_SIZE   16
#define ECB_AES_BLOCK_SIZE 16

#if !defined(CONFIG_ENTROPY_HAS_DRIVER)
#error Entropy driver required for secure random number support
#endif

LOG_MODULE_DECLARE(zboss_osif, CONFIG_ZBOSS_OSIF_LOG_LEVEL);

#if CONFIG_CRYPTO_NRF_ECB
static const struct device *dev;

static void encrypt_aes(zb_uint8_t *key, zb_uint8_t *msg, zb_uint8_t *c)
{
	int err;

	__ASSERT(dev, "encryption call too early");

	struct cipher_ctx ctx = {
		.keylen = ECB_AES_KEY_SIZE,
		.key.bit_stream = key,
		.flags = CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS,
	};
	struct cipher_pkt encryption = {
		.in_buf = msg,
		.in_len = ECB_AES_BLOCK_SIZE,
		.out_buf_max = ECB_AES_BLOCK_SIZE,
		.out_buf = c,
	};

	err = cipher_begin_session(dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				   CRYPTO_CIPHER_MODE_ECB,
				   CRYPTO_CIPHER_OP_ENCRYPT);
	__ASSERT(!err, "Session init failed");

	if (err) {
		goto out;
	}

	err = cipher_block_op(&ctx, &encryption);
	__ASSERT(!err, "Encryption failed");

out:
	cipher_free_session(dev, &ctx);
}
#elif CONFIG_BT_CTLR
static void encrypt_aes(zb_uint8_t *key, zb_uint8_t *msg, zb_uint8_t *c)
{
	int err;

	err = bt_encrypt_be(key, msg, c);
	__ASSERT(!err, "Encryption failed");
}
#elif CONFIG_ZIGBEE_USE_SOFTWARE_AES
static void encrypt_aes(zb_uint8_t *key, zb_uint8_t *msg, zb_uint8_t *c)
{
	int err;
	struct tc_aes_key_sched_struct s;

	err = tc_aes128_set_encrypt_key(&s, key);
	__ASSERT(err == TC_CRYPTO_SUCCESS, "Key set failed");

	err = tc_aes_encrypt(c, msg, &s);
	__ASSERT(err == TC_CRYPTO_SUCCESS, "Encryption failed");
}
#endif

void zb_osif_rng_init(void)
{
}

zb_uint32_t zb_random_seed(void)
{
	zb_uint32_t rnd_val = 0;
	int err_code;

	err_code = sys_csrand_get(&rnd_val, sizeof(rnd_val));
	__ASSERT_NO_MSG(err_code == 0);
	return rnd_val;
}

void zb_osif_aes_init(void)
{
#if CONFIG_CRYPTO_NRF_ECB
	dev = DEVICE_DT_GET(DT_INST(0, nordic_nrf_ecb));
	__ASSERT(device_is_ready(dev), "Crypto driver not found");
#endif
}

void zb_osif_aes128_hw_encrypt(zb_uint8_t *key, zb_uint8_t *msg, zb_uint8_t *c)
{
	if (!(c && msg && key)) {
		__ASSERT(false, "NULL argument passed");
		return;
	}

	encrypt_aes(key, msg, c);
}
