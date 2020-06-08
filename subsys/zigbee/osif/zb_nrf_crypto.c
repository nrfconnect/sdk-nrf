/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <sys/__assert.h>
#include <random/rand32.h>
#include <logging/log.h>
#include <crypto/cipher.h>
#include "zb_nrf_crypto.h"
#include <zboss_api.h>

#define ECB_AES_KEY_SIZE   16
#define ECB_AES_BLOCK_SIZE 16

#if !defined(CONFIG_ENTROPY_HAS_DRIVER)
#error Entropy driver required for secure random number support
#endif

LOG_MODULE_DECLARE(zboss_osif, CONFIG_ZBOSS_OSIF_LOG_LEVEL);

static struct device *dev;

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
	dev = device_get_binding(CONFIG_CRYPTO_NRF_ECB_DRV_NAME);
	__ASSERT(dev, "Crypto driver not found");
}

void zb_osif_aes128_hw_encrypt(zb_uint8_t *key, zb_uint8_t *msg, zb_uint8_t *c)
{
	int err;

	if (!(c && msg && key)) {
		__ASSERT(false, "NULL argument passed");
		return;
	}

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
